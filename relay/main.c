#include "uidpool.h"
#include "bootloader.h"
#include "protocol/dispatcher.h"
#include "config.h"
#include "client.h"
#include "server.h"
#include "preboot.h"
#include "../kernel/bootloader/bootloader.gen.c"
#include "../kernel/dispatcher/dispatcher.gen.c"

#include <ax/reactor.h>
#include <ax/rb.h>
#include <ax/socket.h>
#include <ax/list.h>

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

ax_reactor *g_reactor = NULL;

ax_event g_cnt_listen_event;
ax_event g_svr_listen_event;

void svr_listen_cb(ax_socket fd, short res_flags, void *arg)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	int svr_fd = accept(fd, (struct sockaddr *)&addr, &len);
	if (svr_fd == -1)
		return;
	server_add(svr_fd);

}

void cnt_listen_cb(ax_socket fd, short res_flags, void *arg)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	int cnt_fd = accept(fd, (struct sockaddr *)&addr, &len);
	if (cnt_fd == -1)
		return;
	ax_socket_set_nonblocking(cnt_fd);
	client_add(cnt_fd);
}

int main()
{
	ax_socket_init();
	g_reactor = ax_reactor_create();

	struct sockaddr_in addr;
	int reuse = 1;

	int svr_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(CONFIG_SERVER_PORT);

	reuse = 1;
	setsockopt(svr_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
	if (bind(svr_listen_fd, (struct sockaddr *)&addr, sizeof addr)) {
		ax_perror("Failed to bind port %d, error %d\n", 5555, errno);
		exit(1);
	}

	listen(svr_listen_fd, 128);

	int cnt_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(CONFIG_CLIENT_PORT);

	reuse = 1;
	setsockopt(cnt_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
	if (bind(cnt_listen_fd, (struct sockaddr *)&addr, sizeof addr)) {
		ax_perror("Failed to bind port %d, %s\n", 6666, strerror(errno));
		exit(1);
	}
	listen(cnt_listen_fd, 128);

	g_svr_listen_event = ax_event_make(svr_listen_fd, AX_EV_READ, svr_listen_cb, NULL);
	g_cnt_listen_event = ax_event_make(cnt_listen_fd, AX_EV_READ, cnt_listen_cb, NULL);

	ax_reactor_add(g_reactor, &g_svr_listen_event);
	ax_reactor_add(g_reactor, &g_cnt_listen_event);

	if (server_startup(g_reactor)) {
		ax_perror("server_startup, %s", strerror(errno));
		exit(1);
	}

	if (client_startup(g_reactor)) {
		ax_perror("client_startup, %s", strerror(errno));
		exit(1);
	}

	if (preboot_startup(g_reactor)) {
		ax_perror("preboot_startup, %s", strerror(errno));
		exit(1);
	}

	preboot_set_stranger_state(PREBOOT_STATE_BOOT);

	if (preboot_set_server_code(dispatcher_bin, sizeof dispatcher_bin)) {
		ax_perror("preboot_set_server_code, %s", strerror(errno));
		exit(1);
	}

	if (bootloader_update_addr(bootloader_bin)) {
		ax_perror("update bootloader failed");
		exit(1);
	}
	if (preboot_set_bootloader_code(bootloader_bin, sizeof bootloader_bin)) {
		ax_perror("preboot_set_bootloader_code, %s", strerror(errno));
		exit(1);
	}

	client_set_cb(server_list_token, server_remove, server_connect_tunnel);

	ax_reactor_loop(g_reactor, NULL, 0);

	return 0;
}

