#ifndef SERVER_H
#define SERVER_H
#include <ax/reactor.h>
#include <ax/list.h>
int server_startup(ax_reactor *reactor);
int server_connect_tunnel(uint32_t token, int tunnel, ax_socket sock, int *retcode);
int server_add(ax_socket sock);

int server_list_token(ax_list_r list, int *errcode);

int server_remove(uint32_t token, int *errcode);
#endif
