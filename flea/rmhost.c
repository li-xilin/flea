#include "tunnel.h"
#include "config.h"
#include "protocol/dispatcher.h"
#include <ax/event.h>
#include <ax/option.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>


static int relay_rmhost(ax_socket relay, uint32_t token, int *err)
{
	uint8_t id = MSG_RLY_RMSVR;
	if (ax_socket_syncsend(relay, &id, sizeof id)) {
		return -1;
	}

	if (ax_socket_syncsend(relay, &token, sizeof token)) {
		return -1;
	}
	
	if (ax_socket_syncrecv(relay, &id, sizeof id)) {
		return -1;
	}

	if (id != MSG_RLY_RMSVR)
		return -1;

	struct msg_ret_code ret;

	if (ax_socket_syncrecv(relay, &ret, sizeof ret)) {
		ax_perror("error: %s", strerror(errno));
		return -1;
	}
	*err = ret.error_code;
	return 0;
}

static void usage()
{
	fputs("Usage: flea rmhost OPTIONS... HOSTS...\n\n"
	      "Options:\n"
	      "    Remove zombie HOSTS which connected to relay.\n\n"
	      "    -r <address>  specifies relay server address\n"
	      "    -p <keyword>  specifies relay keyword\n"
	, stderr);
}

int cmd_rmhost(int argc, char **argv)
{
	struct ax_option_st options;
	ax_option_init(&options, argv);
	const char *relay_addr = NULL;
	const char *keyword = NULL;
	for (int op; (op = ax_option_parse(&options, "r:p:h")) != -1;) {
		switch (op) {
			case 'r':
				relay_addr = options.optarg;
				break;
			case 'p':
				keyword = options.optarg;
				break;
			case 'h':
				usage();
				return 0;
			default:
				fprintf(stderr, "Unsupported option -%c\n", options.optopt);
				usage();
				return 1;
		}
	}

	if (argc - options.optind == 0) {
		ax_perror("no mod is specified");
		usage();
		return 1;
	}


	if (!relay_addr) {
		ax_perror("Option -r expected");
		return 1;
	}

	if (!keyword) {
		fprintf(stderr, "Option -p expected");
		return 1;
	}

	ax_socket relay = tunnel_open(keyword, relay_addr, CONFIG_CLIENT_PORT, 0, 0);
	if (relay == AX_SOCKET_INVALID) {
		ax_perror("Failed to connect to relay service");
		return 1;
	}

	for (int i = options.optind; i < argc; i++) {
		uint32_t token_id;
		if (sscanf(argv[i], "%" PRIu32, &token_id) != 1 || token_id == 0) {
			ax_perror("Invalid host id %s", argv[i]);
			continue;
		}
		int err;
		relay_rmhost(relay, token_id, &err);

		if (err) {
			ax_pwarn("Remove host %s failed, error %d", argv[i], err);
		}
	}
	
	ax_socket_close(relay);
	return 0;
}
