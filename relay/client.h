#ifndef CLIENT_H
#define CLIENT_H
#include <ax/reactor.h>
#include <ax/list.h>
typedef int list_server_cb(ax_list_r svr_list, int *errcode);
typedef int remove_server_cb(uint32_t token, int *errcode);
typedef int tunnel_accept_cb(uint32_t token, int tunnel, ax_socket sock, int *errcode);

int client_startup(ax_reactor *reactor);

int client_add(ax_socket sock);

void client_set_cb(list_server_cb *list_server_cb,
		remove_server_cb *remove_server_cb, tunnel_accept_cb *tunnel_accept_cb);
#endif
