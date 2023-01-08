#include "uidpool.h"
#include "protocol/dispatcher.h"
#include "server.h"

#include <ax/reactor.h>
#include <ax/rb.h>
#include <ax/list.h>
#include <ax/socket.h>

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define TUNNEL_MAX_COUNT 16

#define INCOME_TIMEOUT 2000

static ax_reactor *g_reactor = NULL;

ax_trait_declare(session, struct session);
ax_trait_declare(income, struct income);

static ax_rb_r g_income_map = AX_R_NULL;
static ax_rb_r g_session_map = AX_R_NULL;
static uidpool *g_token_pool = NULL;

struct tunnel {
	ax_event event;
	ax_event peer_event;
	bool connected;
	uint32_t token;
	uint8_t tunnel;
};

struct session {
	struct tunnel *tunnels[TUNNEL_MAX_COUNT];
	struct in_addr inaddr;
	int token;
};

struct income {
	ax_event event;
	ax_event timeout_event;
	struct msg_hello request;
	unsigned int received;
	time_t time;
};

static void tunnel_read_cb(ax_socket fd, short res_flags, void *arg);
static void income_read_cb(ax_socket fd, short res_flags, void *arg);
static void income_timeout_cb(ax_socket fd, short res_flags, void *arg);

static int tunnel_create(struct session *sess, int tunnel, ax_socket sock)
{
	assert(sess->tunnels[tunnel] == NULL);
	sess->tunnels[tunnel] = (struct tunnel *)malloc(sizeof(struct tunnel));
	if (!sess->tunnels[tunnel])
		return -1;

	sess->tunnels[tunnel]->token = sess->token;
	sess->tunnels[tunnel]->tunnel = tunnel;
	sess->tunnels[tunnel]->connected = false;
	sess->tunnels[tunnel]->event = ax_event_make(sock, AX_EV_READ,
			tunnel_read_cb, sess->tunnels[tunnel]);
	ax_reactor_add(g_reactor, &sess->tunnels[tunnel]->event);
	return 0;
}

ax_fail session_init(void* p, va_list *ap) // fd, token
{
	assert(ap);
	int fd = va_arg(*ap, int);
	uint32_t token = va_arg(*ap, uint32_t);

	struct session *sess = (struct session *)p;
	for (int i = 0; i < TUNNEL_MAX_COUNT; i++)
		sess->tunnels[i] = NULL;
	sess->token = token;

	struct sockaddr_in inaddr;
	socklen_t socklen;
	getpeername(fd, (struct sockaddr *)&inaddr, &socklen);
	sess->inaddr = inaddr.sin_addr;

	if (tunnel_create(sess, 0, fd))
		return true;

	return false;
}

void session_free(void* p)
{
	struct session *sess = (struct session *)p;
	if (ax_event_in_use(&sess->tunnels[0]->event))
		ax_reactor_remove(g_reactor, &sess->tunnels[0]->event);
	free(sess->tunnels[0]);
	sess->tunnels[0] = NULL;
}

ax_trait_define(session, INIT(session_init), FREE(session_free));

static ax_fail income_init(void* p, va_list *ap) // fd
{
	assert(ap);
	struct income *income = (struct income *)p;
	int fd = va_arg(*ap, int);
	
	income->event = ax_event_make(fd, AX_EV_READ, income_read_cb, p);
	ax_reactor_add(g_reactor, &income->event);

	income->timeout_event = ax_event_make(INCOME_TIMEOUT, AX_EV_TIMEOUT, income_timeout_cb, p);
	ax_reactor_add(g_reactor, &income->timeout_event);
	// income->time = time(NULL);
	income->received = 0;
	return false;
}

static void income_free(void* p)
{
	struct income *income = (struct income *)p;
	ax_reactor_remove(g_reactor, &income->event);
	ax_reactor_remove(g_reactor, &income->timeout_event);
}

ax_trait_define(income, INIT(income_init), FREE(income_free));

static void close_tunnel(struct tunnel *tun)
{
	if (!tun)
		return;
	assert(tun->tunnel != 0);
	struct session *sess = ax_map_get(g_session_map.ax_map, &tun->token);
	
	if (tun->connected) {
		ax_reactor_remove(g_reactor, &tun->peer_event);
		ax_socket_close(tun->peer_event.fd);
	}
	int fd = tun->event.fd;
	if (ax_event_in_use(&tun->event))
		ax_reactor_remove(g_reactor, &tun->event);
	ax_socket_close(fd);

	int tunnel = tun->tunnel;
	free(sess->tunnels[tunnel]);
	sess->tunnels[tunnel] = NULL;
}

int server_remove(uint32_t token, int *errcode)
{
	struct session *sess = ax_map_get(g_session_map.ax_map, &token);
	if (!sess) {
		if (errcode)
			*errcode = MSG_EILTOK;
		return -1;
	}
	for (int i = 1; i < TUNNEL_MAX_COUNT; i++)
		close_tunnel(sess->tunnels[i]); //Do not close tunnel 0
	ax_socket_close(sess->tunnels[0]->event.fd);
	ax_map_erase(g_session_map.ax_map, &token);
	return 0;
}

static void tunnel_read_cb(ax_socket fd, short res_flags, void *arg)
{
	struct tunnel *tun= (struct tunnel *)arg;
	// assert(tun->connected);
	ssize_t len;
	if (!tun->connected) {
		char dummy;
		len = recv(fd, &dummy, sizeof dummy, MSG_PEEK);
		if (len <= 0) {
			server_remove(tun->token, NULL);
		} else {
			ax_reactor_remove(g_reactor, &tun->event);
		}
		return;
	}

	char recv_buf[1024];
	len = recv(fd, &recv_buf, sizeof recv_buf, 0);

	if (len == 0 || len == -1) {
		if (len == -1 && errno == EWOULDBLOCK)
			return;
		if (tun->tunnel == 0)
			server_remove(tun->token, NULL);
		else
			close_tunnel(tun);
		return;
	}
	
	send(tun->peer_event.fd, recv_buf, len, MSG_NOSIGNAL);
}

static void income_read_cb(ax_socket fd, short res_flags, void *arg)
{
	struct income *income = arg;

	ssize_t len = recv(fd, &income->request, sizeof(struct msg_hello) - income->received, 0);
	if (len == -1) {
		if (errno == EINTR)
			return;
		ax_map_erase(g_income_map.ax_map, &fd);
		ax_socket_close(fd);
		ax_pwarn("recv: %d", strerror(errno));
		return;
	} 

	if (len == 0) {
		ax_map_erase(g_income_map.ax_map, &fd);
		ax_socket_close(fd);
		return;
	}

	income->received += len;

	if (income->received != sizeof(struct msg_hello))
		return;

	ax_map_erase(g_income_map.ax_map, &fd);
	
	int tunnel = income->request.tunnel;
	int token = income->request.token;

	struct msg_reply_hello reply;
	if (income->request.token == 0) {

		int token = uidpool_get(g_token_pool);
		ax_map_iput(g_session_map.ax_map, &token, fd, token);
		reply.token = token;
		send(fd, &reply, sizeof(reply), MSG_NOSIGNAL);
		return;
	}
	struct session *sess = (struct session *)
		ax_map_get(g_session_map.ax_map, &token);
	if (!sess || tunnel >= TUNNEL_MAX_COUNT || tunnel == 0) {
		ax_socket_close(fd);
		return;
	}
	if (sess->tunnels[tunnel] != NULL) {
		ax_pwarn("Tunnel already in use: token = %d, tunnel = %d", token, tunnel);
		ax_socket_close(fd);
		return;
	}

	if (tunnel_create(sess, tunnel, fd)) {
		ax_socket_close(fd);
		return;
	}

	reply.token = 0;
	send(fd, &reply, sizeof(reply), MSG_NOSIGNAL);
	return;
}


static void income_timeout_cb(ax_socket fd, short res_flags, void *arg)
{
	struct income *income = arg;
	ax_socket sock = income->event.fd;
	ax_map_erase(g_income_map.ax_map, &sock);
	ax_socket_close(income->event.fd);
}

void peer_read_cb(ax_socket fd, short res_flags, void *arg)
{
	struct tunnel *tun= (struct tunnel *)arg;
	assert(tun->connected);

	char recv_buf[1024];
	ssize_t len;
	len = recv(fd, &recv_buf, sizeof recv_buf, 0);

	if (len == 0 || len == -1) {
		if (len == -1 && errno == EWOULDBLOCK)
			return;

		if (tun->tunnel == 0) {
			// server_remove(tun->token, NULL);
			tun->connected = false;
			ax_socket_close(tun->peer_event.fd);
			ax_reactor_remove(g_reactor, &tun->event);
			ax_reactor_remove(g_reactor, &tun->peer_event);
		}
		else
			close_tunnel(tun);
		return;
	}
	
	send(tun->event.fd, recv_buf, len, MSG_NOSIGNAL);
}

int server_startup(ax_reactor *reactor)
{
	g_token_pool = uidpool_create();
	g_session_map = ax_new(ax_rb, ax_t(int), ax_t(session));
	g_income_map = ax_new(ax_rb, ax_t(int), ax_t(income));
	g_reactor = reactor;
	return 0;
}

int server_connect_tunnel(uint32_t token, int tunnel, ax_socket sock, int *retcode)
{
	assert(tunnel >= 0 && tunnel < TUNNEL_MAX_COUNT);
	struct session *sess = ax_map_get(g_session_map.ax_map, &token);
	if (!sess) {
		*retcode = MSG_EILTOK;
		return -1;
	}
	if (!sess->tunnels[tunnel]) {
		*retcode = MSG_EILTUN;
		return -1;
	}
	if (sess->tunnels[tunnel]->connected) {
		*retcode = MSG_ETUNINUSE;
		return -1;
	}

	sess->tunnels[tunnel]->peer_event = ax_event_make(sock, AX_EV_READ, peer_read_cb, sess->tunnels[tunnel]);
	sess->tunnels[tunnel]->connected = true;

	if (!ax_event_in_use(&sess->tunnels[tunnel]->event))
		ax_reactor_add(g_reactor, &sess->tunnels[tunnel]->event);
	ax_reactor_add(g_reactor, &sess->tunnels[tunnel]->peer_event);
	*retcode = MSG_EOK;
	return 0;
}

int server_add(ax_socket sock)
{
	if (!ax_map_iput(g_income_map.ax_map, &sock, sock))
		return -1;
	return 0;
}

int server_list_token(ax_list_r list, int *errcode)
{
	ax_map_cforeach(g_session_map.ax_map, const uint32_t *,
			token, const struct session *, sess) {
		if (ax_seq_ipush(list.ax_seq, *token, sess->inaddr.s_addr)) {
			*errcode = MSG_ENOMEM;
			return -1;
		}
	}
	*errcode = MSG_EOK;
	return 0;
}


