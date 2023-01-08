#include "terminal/flea.h"
#include <stdio.h>
#include <stdlib.h>

int flea_main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;
	int32_t token = atoi(argv[1]);
	ax_socket sock = flea_tunnel_open(token, 5);
	if (sock == AX_SOCKET_INVALID) {
		ax_perror("flea_tunnel_open");
		return 1;
	}
	char buf[1024];
	recv(sock, buf, sizeof buf, MSG_NOSIGNAL);

	puts(buf);

	ax_socket_close(sock);
	return 0;
}
