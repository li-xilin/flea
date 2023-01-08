#include "terminal/flea.h"
#include "../protocol.h"
#include "protocol/common.h"

#include <ax/socket.h>
#include <ax/option.h>
#include <ax/flow.h>
#include <ax/mem.h>
#include <ax/edit.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <libgen.h>
#include <assert.h>
#include <time.h>

#define WEBCAM_TUNNEL 4

static int webcam_capture(ax_socket sock, int device)
{
	int retval = 0;
	FILE *fp = NULL;

	time_t t = time(NULL);
	struct tm* stime = localtime(&t);
	char filename[NAME_MAX];
	sprintf(filename, "%04d%02d%02d-%02d%02d%02d-%04d.jpg",
			1900 + stime->tm_year, 1 + stime->tm_mon, stime->tm_mday,
			stime->tm_hour, stime->tm_min,stime->tm_sec,
			rand() % 10000);

	fp = fopen(filename, "wb");
        if (!fp) {
                fprintf(stderr, "%s: failed to open local file: %s\n", flea_module_name(), strerror(errno));
                goto out;
        }

	uint8_t id = WEBCAM_MSG_CAPTURE;
	if (ax_socket_syncsend(sock, &id, sizeof id)) {
		retval = -1;
		goto out;
	}

	uint8_t index_u8 = (uint8_t)device;
	if (ax_socket_syncsend(sock, &index_u8, sizeof index_u8)) {
		retval = -1;
		goto out;
	}
	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		retval = -1;
		goto out;
	}
	if (ret.error_code) {
		flea_perror(ret.error_code);
		goto out;
	}

	uint32_t file_size;
	if (ax_socket_syncrecv(sock, &file_size, sizeof file_size)) {
		retval = -1;
		return -1;
	}

        bool failed = false;
        size_t received = 0;
        while (received != file_size) {
                char buf[4096];
		size_t to_recv = file_size - received;
		if (to_recv > sizeof buf)
			to_recv = sizeof buf;
                if (ax_socket_syncrecv(sock, buf, to_recv)) {
                        retval = -1;
                        goto out;
                }
                if (!failed && fwrite(buf, 1, to_recv, fp) != to_recv) {
                        failed = true;
                        fprintf(stderr, "put: %s\n", strerror(errno));
                        uint8_t cancel = 1;
                        if (ax_socket_syncsend(sock, &cancel, sizeof cancel))
                                retval = -1;
                } else {
                        received += to_recv;
		}
        }
	printf("saved as %s\n", filename);
out:
	if (fp)
		fclose(fp);
	return retval;
}

static int webcam_list(ax_socket sock)
{
	int retval = -1;

	uint8_t id = WEBCAM_MSG_LIST_DEVICE;
	if (ax_socket_syncsend(sock, &id, sizeof id)) {
		retval = -1;
		goto out;
	}

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		retval = -1;
		goto out;
	}
	if (ret.error_code) {
		flea_perror(ret.error_code);
		goto out;
	}

	for (int i = 0; ; i++) {
		uint8_t name_size;
		if (ax_socket_syncrecv(sock, &name_size, sizeof name_size)) {
			retval = -1;
			goto out;
		}
		if (!name_size)
			break;
		char name[0xFF];
		if (ax_socket_syncrecv(sock, name, name_size)) {
			retval = -1;
			goto out;
		}
		printf("%d - %s\n", i, name);
	}

out:
	return 0;
}

void usage()
{
}

static void completion_hook(char const *buf, ax_edit_completions *pCompletion)
{
	int i;
	static const char *cmd[] = {"list", "capture", NULL};

	for (i = 0; NULL != cmd[i]; i++) {
		if (0 == strncmp(buf, cmd[i], strlen(buf))) {
			ax_edit_completion_add(pCompletion, cmd[i], NULL);
		}
	}
}

int flea_main(int argc, char *argv[])
{
	int retval = 0;
	if (argc < 2)
		return 1;
	int32_t token = atoi(argv[1]);

	ax_socket sock = flea_tunnel_open(token, WEBCAM_TUNNEL);
	if (sock == AX_SOCKET_INVALID) {
		ax_perror("flea_tunnel_open");
		return 1;
	}

	srand(time(NULL));

	ax_forever {
		char ps1[PATH_MAX];
		sprintf(ps1, "[%s:%d]# ", argv[0], token);

		char prompt_line[1024];

		ax_edit_completion_register(completion_hook);

		if (ax_edit_readline(ps1, prompt_line, sizeof(prompt_line)) == NULL)
			continue;

		int line_length = strlen(prompt_line);
		if (prompt_line[line_length - 1] == '\n')
			prompt_line[line_length - 1] = '\0';

		char *prompt_argv[32];
		int prompt_argc = ax_argv_from_buf(prompt_line, prompt_argv, sizeof prompt_argv / sizeof *prompt_argv);
		if (prompt_argc < 1)
			continue;

		if (strcmp(prompt_argv[0], "capture") == 0) {
			if (prompt_argc < 2) {
				fputs("capture: INDEX is not specified.\n", stderr);
				continue;
			}
			int index;
			if (sscanf(prompt_argv[1], "%d", &index) != 1 || index > 0xFF) {
				fputs("capture: INDEX is invalid.\n", stderr);
				continue;
			}
			if (webcam_capture(sock, index)) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "list") == 0) {
			if (webcam_list(sock)) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "exit") == 0)
			break;
		else if (strcmp(prompt_argv[0], "q") == 0)
			break;
		else {
			fprintf(stderr, "%s: unknown command `%s'\n", flea_module_name(), prompt_argv[0]);
			continue;
		}
	}
out:
	ax_socket_close(sock);
	return 0;
}

int flea_stop()
{
	puts("ctrl + c ...");
	return 0;
}
