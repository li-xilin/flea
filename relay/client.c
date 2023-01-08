#include "rstack.h"
#include "client.h"
#include "hostinf.h"
#include "config.h"
#include "protocol/dispatcher.h"

#include <ax/reactor.h>
#include <ax/rb.h>
#include <ax/socket.h>
#include <ax/list.h>

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

static list_server_cb *g_list_server_cb;
static remove_server_cb *g_remove_server_cb;
static tunnel_accept_cb *g_tunnel_accept_cb;
static struct rstack *g_rst;
static ax_reactor *g_reactor = NULL;
static ax_rb_r g_cnt_sess_map = AX_R_NULL;

ax_trait_declare(cnt_sess, struct cnt_sess);

struct cnt_sess {
	ax_event event;
	uint32_t token;
	uint8_t tunnel;
	bool logon;
};


void cnt_read_cb(ax_socket fd, short res_flags, void *arg)
{
	int len;
	uint8_t id;
	char recv_buf[1024];

	struct cnt_sess *sess = arg;
	if (sess->logon == false) {
		struct msg_cnt_login login;
		struct msg_ret_code ret_code;
		len = recv(fd, &login, sizeof login, MSG_PEEK);
		if (len == 0 || len == -1) {
			if (len == -1 && errno == EWOULDBLOCK)
				return;
			ax_map_erase(g_cnt_sess_map.ax_map, &fd);
			ax_socket_close(fd);
			return;
		}
		if (len != sizeof login)
			return;

		len = recv(fd, &login, sizeof login, 0);
		if (strncmp(login.password, CONFIG_RELAY_PASSWORD, sizeof login.password) != 0) {
			ret_code.error_code = 1;
			send(fd, &ret_code, sizeof ret_code, MSG_NOSIGNAL);
			ax_map_erase(g_cnt_sess_map.ax_map, &fd);
			ax_socket_close(fd);
			return;
		}
		// 1 判定Tunnel是否存在
		// 2 将客户端连接绑定该tunnel
		if (login.token != 0) { //连接Tunnel
			int errcode;
			ax_map_erase(g_cnt_sess_map.ax_map, &fd);
			g_tunnel_accept_cb(login.token, login.tunnel, fd, &errcode);
			ret_code.error_code = errcode;
			send(fd, &ret_code, sizeof ret_code, MSG_NOSIGNAL);
			return;
		}

		// check token
		sess->token = login.token;
		sess->tunnel = login.tunnel;
		sess->logon = true;

		ret_code.error_code = 0;
		send(fd, &ret_code, sizeof ret_code, MSG_NOSIGNAL);
		return;
	}

	if (sess->token == 0) { // Relay
		size_t size;
		char *buf = rstack_get_buffer(g_rst, &size);
		len = 0;
		if (size) {
			len = recv(fd, buf, size, 0);
			if (len == 0) {
				ax_map_erase(g_cnt_sess_map.ax_map, &fd);
				return;
			}
			if (len == -1) { 
				if (errno != EWOULDBLOCK) {
					ax_map_erase(g_cnt_sess_map.ax_map, &fd);
					ax_socket_close(fd);
				}
				return;
			}
		}
		if (!rstack_notify(g_rst, len))
			return;

		int depth;
		const struct rstack_frame *frame = rstack_get_frame(g_rst, &depth);


		if (depth == 1 && frame[0].data[0] == MSG_RLY_LSSVR) {
			send(fd, &frame[0].data[0], 1, 0);
			struct msg_host_set set;
			int errcode;
			ax_list_r list = ax_new(ax_list, ax_t(hostinf));
			if (ax_r_isnull(list)) {
				set.count = 0;
			}
			else if (g_list_server_cb(list, &errcode)) {
				set.count = 0;
			}
			set.count = ax_box_size(list.ax_box);
			send(fd, &set, sizeof set, MSG_NOSIGNAL);
			ax_box_cforeach(list.ax_box, const struct hostinf *, hostinf) {
				struct msg_host_info host;
				host.token = hostinf->token;
				host.inaddr = hostinf->inaddr;
				send(fd, &host, sizeof host, MSG_NOSIGNAL);
			}
			if (!ax_r_isnull(list))
				ax_one_free(list.ax_one);
		}

		else if (depth == 2 && frame[0].data[0] == MSG_RLY_RMSVR) {
			send(fd, &frame[0].data[0], 1, 0);
			uint32_t *ptoken = (uint32_t *)(frame[1].data);
			struct msg_ret_code retcode = { .error_code = MSG_EOK };
			int errcode;

			if (g_remove_server_cb(*ptoken, &errcode))
				retcode.error_code = errcode;;

			ssize_t len = send(fd, &retcode, sizeof retcode, MSG_NOSIGNAL);
			if (len == -1) {
				ax_map_erase(g_cnt_sess_map.ax_map, &fd);
				ax_socket_close(fd);
			}
		}
		rstack_commit(g_rst, 0);
	}
}

ax_fail cnt_sess_init(void* p, va_list *ap) // fd
{
	assert(ap);
	struct cnt_sess *sess = (struct cnt_sess *)p;
	int fd = va_arg(*ap, int);
	sess->event = ax_event_make(fd, AX_EV_READ, cnt_read_cb, sess);
	sess->token = 0;
	sess->tunnel = 0;
	sess->logon = false;
	ax_reactor_add(g_reactor, &sess->event);
	return false;
}

void cnt_sess_free(void* p)
{
	struct cnt_sess *sess = (struct cnt_sess *)p;
	ax_reactor_remove(g_reactor, &sess->event);
}

ax_trait_define(cnt_sess, INIT(cnt_sess_init), FREE(cnt_sess_free));

typedef int list_server_cb(ax_list_r svr_list, int *errcode);
typedef int remove_server_cb(uint32_t token, int *errcode);
typedef int tunnel_accept_cb(uint32_t token, int tunnel, ax_socket sock, int *errcode);

enum rstack_next_type next_cb(struct rstack *rst, size_t *size)
{
	int depth;
	const struct rstack_frame *frm = rstack_get_frame(rst, &depth);
	if (depth == 0) {
		*size = 1;
		return RSTACK_NT_BLOCK;
	}

	if (depth == 1 && frm[0].data[0] == MSG_RLY_LSSVR) {
		*size = 0;
		return RSTACK_NT_RESET;
	}

	if (depth == 1 && frm[0].data[0] == MSG_RLY_RMSVR) {
		*size = 4;
		return RSTACK_NT_BLOCK;
	}

	if (depth == 2 && frm[0].data[0] == MSG_RLY_RMSVR) {
		*size = 0;
		return RSTACK_NT_RESET;
	}
	return -1;
}

int client_startup(ax_reactor *reactor)
{
	g_cnt_sess_map = ax_new(ax_rb, ax_t(int), ax_t(cnt_sess));
	g_rst = rstack_init(next_cb);
	g_reactor = reactor;
	return 0;
}

void client_set_cb(list_server_cb *list_server_cb,
		remove_server_cb *remove_server_cb, tunnel_accept_cb *tunnel_accept_cb) {
	g_list_server_cb = list_server_cb;
	g_remove_server_cb = remove_server_cb;
	g_tunnel_accept_cb = tunnel_accept_cb;
}

int client_add(ax_socket sock)
{
        if (!ax_map_iput(g_cnt_sess_map.ax_map, &sock, sock))
                return -1;
        return 0;
}
