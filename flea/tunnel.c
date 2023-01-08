#include "tunnel.h"
#include "protocol/dispatcher.h"
#include <netdb.h>
#include <errno.h>

ax_socket tunnel_open(const char *keyword, const char *relay_host, uint16_t relay_port, uint32_t token, int tunnel)
{
	int fd = -1;
	if (strlen(keyword) > PROTO_PASSWORD_MAX - 1)
		return -1;
	struct hostent *host = gethostbyname(relay_host);
	if (!host)
		return -1;
	if (host->h_length == 0)
		return -1;
	struct in_addr inaddr = *(struct in_addr *)host->h_addr_list[0];;

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in sockaddr;

        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = inaddr.s_addr;
        sockaddr.sin_port = htons(relay_port);;

	
	// struct sockaddr_in peeraddr;
	if (connect(fd, (struct sockaddr *)&sockaddr, sizeof sockaddr))
		goto fail;
	struct msg_cnt_login login;
	strcpy(login.password, keyword);
	login.token = token;
	login.tunnel = tunnel;
	if (ax_socket_syncsend(fd, &login, sizeof login))
		goto fail;
	
	struct msg_ret_code ret;

	if (ax_socket_syncrecv(fd, &ret, sizeof ret))
		goto fail;
	if (ret.error_code)
		goto fail;
	return fd;
fail:
	ax_socket_close(fd);
	return -1;
}
