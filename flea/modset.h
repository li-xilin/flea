#ifndef MODMGR_H
#define MODMGR_H

struct mod
{
	enum mod_type {
		MOD_TYPE_BIN,
		MOD_TYPE_DLL,
		MOD_TYPE_EXE,
	} type;
	const char *brief;
	const char *name;
	const char *directory;
	const char *server_mod;
	const char *client_mod;
};

int modset_init(const char *dir);
void modset_deinit();
struct mod *modset_find(const char *name);
const char *modset_dir();

#endif
