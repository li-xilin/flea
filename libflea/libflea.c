#include "terminal/flea.h"
#include "protocol/dispatcher.h"
#include <ax/mem.h>
#include <ax/option.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

char g_keyword[PROTO_PASSWORD_MAX];
char g_mod_name[MOD_NAME_MAX];
struct sockaddr_in g_relay_addr = { .sin_family = AF_INET };

static ax_socket __tunnel_open(const char *keyword, struct sockaddr_in *relay_addr, uint32_t token, int tunnel)
{
        int fd = -1;
        if (strlen(keyword) > PROTO_PASSWORD_MAX - 1)
                return -1;
        

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        // struct sockaddr_in peeraddr;
        if (connect(fd, (struct sockaddr *)relay_addr, sizeof *relay_addr))
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

extern int flea_main(int argc, char *argv[]);

ax_socket flea_tunnel_open(uint32_t token, int tunnel)
{
	return __tunnel_open(g_keyword, &g_relay_addr, token, tunnel);
}

const char *flea_module_name()
{
	return g_mod_name;
}

int flea_start(const char *modname, const char *keyword, const char *relay_host, uint16_t port,char *arg_line)
{
	if (strlen(modname) + 1 > MOD_NAME_MAX) {
		fprintf(stderr, "module name is too long");
		return -1;
	}
	strcpy(g_mod_name, modname);

	if (strlen(keyword) > PROTO_PASSWORD_MAX - 1)
		return -1;
	strcpy(g_keyword, keyword);

	struct hostent *host = gethostbyname(relay_host);
	if (!host)
		return -1;
	if (host->h_length == 0)
		return 1;
	g_relay_addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];

	g_relay_addr.sin_port = htons(port);

	char *argv[0xFF];
	argv[0] = g_mod_name;
	int argc = ax_argv_from_buf(arg_line, argv + 1, sizeof argv / sizeof *argv - 1);

	return flea_main(argc + 1, argv);

	return 0;
}

