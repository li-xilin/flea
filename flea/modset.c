#include "modset.h"
#include <iniparser/iniparser.h>
#include <ax/rb.h>
#include <ax/log.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

ax_rb_r g_mod_map = AX_R_NULL;
char *g_filepath = NULL;

ax_trait_declare(mod, struct mod);

ax_fail mod_copy(void *dst, const void *src)
{
	struct mod *d = dst;
	const struct mod *s = src;
	d->name = d->brief = d->client_mod = d->server_mod = NULL;
	d->type = s->type;
	if (!(d->directory = ax_strdup(s->directory)))
		goto fail;
	if (!(d->name = ax_strdup(s->name)))
		goto fail;
	if (!(d->brief = ax_strdup(s->brief)))
		goto fail;
	if (s->client_mod) {
		if (!(d->client_mod = ax_strdup(s->client_mod)))
			goto fail;
	}
	else
		d->client_mod = NULL;

	if (!(d->server_mod = ax_strdup(s->server_mod)))
		goto fail;
	return false;
fail:
	free((void *)d->name);
	free((void *)d->directory);
	free((void *)d->brief);
	free((void *)d->client_mod);
	free((void *)d->server_mod);
	return true;
}

void mod_free(void *p)
{
	struct mod *mod = p;
	free((void *)mod->name);
	free((void *)mod->directory);
	free((void *)mod->brief);
	free((void *)mod->client_mod);
	free((void *)mod->server_mod);
}

ax_trait_define(mod, COPY(mod_copy), FREE(mod_free));

static int load_modini(dictionary *ini, struct mod *mod)
{
	if (!(mod->name = iniparser_getstring(ini,"Module:Name", NULL)))
		return -1;
	if (!(mod->brief = iniparser_getstring(ini,"Module:Brief", NULL))) {
		mod->brief = mod->name;
	}
	if (!(mod->server_mod = iniparser_getstring(ini,"Module:ServerFile", NULL)))
		return -1;
	mod->client_mod = iniparser_getstring(ini,"Module:ClientFile", NULL);
	return 0;
}

int modset_init(const char *mod_path)
{
	g_mod_map = ax_new(ax_rb, ax_t(str), ax_t(mod));
	if (ax_r_isnull(g_mod_map))
		goto fail;

	g_filepath = ax_strdup(mod_path);
	if (!g_filepath)
		goto fail;

	DIR *dir;
	if ((dir=opendir(mod_path)) == NULL) {
		ax_perror("Open directory failed: %s", strerror(errno));
		goto fail;
	}

	char path_buf[PATH_MAX];
	size_t mod_path_len = strlen(mod_path);
	memcpy(path_buf, mod_path, mod_path_len);
	path_buf[mod_path_len] = '/';
	mod_path_len++;

	struct dirent *ptr;
	while ((ptr=readdir(dir)) != NULL)
	{
		if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
			continue;
		if(ptr->d_type == DT_DIR) {
			strcpy(path_buf + mod_path_len, ptr->d_name);
			strcat(path_buf, "/mod.ini");

			dictionary *ini = iniparser_load(path_buf);
			if (!ini) {
				ax_perror("Failed to load mod.ini from module %s: %s",
						ptr->d_name, strerror(errno));
				continue;
			}
			struct mod mod;
			if (load_modini(ini, &mod)) {
				iniparser_freedict(ini);
				continue;
			}
			mod.directory = ptr->d_name;
			if (ax_map_exist(g_mod_map.ax_map, mod.name)) {
				iniparser_freedict(ini);
				ax_pwarn("Duplicated module name %s, ignored.", mod.name);
				continue;
			}
			if (!ax_map_put(g_mod_map.ax_map, mod.name, &mod))
				ax_perror("Load module failed: %d", strerror(errno));
			iniparser_freedict(ini);
		}
	}
	closedir(dir);
	return 0;
fail:
	if (dir)
		closedir(dir);
	if (g_filepath) {
		free(g_filepath);
		g_filepath = NULL;
	}
	if (g_mod_map.ax_one) {
		ax_one_free(g_mod_map.ax_one);
		g_mod_map.ax_one = NULL;
	}
	return -1;
}

void modset_deinit()
{
	assert(!ax_r_isnull(g_mod_map));
	ax_one_free(g_mod_map.ax_one);
	g_mod_map.ax_one = NULL;
}

struct mod *modset_find(const char *name)
{
	assert(!ax_r_isnull(g_mod_map));
	struct mod *m = ax_map_get(g_mod_map.ax_map, name);
	if (!m) {
		ax_perror("module %s is not found", name);
		return NULL;
	}
	return m;
}

const char *modset_dir()
{
	assert(!ax_r_isnull(g_mod_map));
	return g_filepath;
}
