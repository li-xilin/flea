#include "../protocol.h"
#include "terminal/flea.h"
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

static char g_current_dir[FS_MAX_PATH];

int list_file(ax_socket sock, const char *path)
{
	uint8_t id = FS_MSG_LIST;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		return -1;

	uint16_t path_len = strlen(path) + 1;
	if (ax_socket_syncsend(sock, &path_len, sizeof path_len))
		return -1;
	if (ax_socket_syncsend(sock, path, path_len))
		return -1;

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret))
		return -1;
	if (ret.error_code) {
		flea_perror(ret.error_code);
		return 0;
	}

	uint32_t file_count;
	if (ax_socket_syncrecv(sock, &file_count, sizeof file_count))
		return -1;

	for (int i = 0; i < file_count; i++) {
		struct fs_msg_file_entry entry;
		if (ax_socket_syncrecv(sock, &entry, sizeof entry))
			return -1;

		char file_name[FILENAME_MAX];
		if (ax_socket_syncrecv(sock, &file_name, entry.name_size))
			return -1;
		uint32_t size = entry.file_size;
		char *unit_tab[] = { "", "KB", "MB", "GB", "TB" };
		int unit;
		for (unit = 0; unit < 5 && size > 1024; unit++) {
			size /= 1024;
		}
		char size_text[32];
		sprintf(size_text, "%" PRIu32 "%s", size, unit_tab[unit]);

		printf("%c %10s %s\n",
				entry.is_dir ? 'd' : ' ',
				size_text,
				file_name);
	}
	return 0;
}

int change_dir(ax_socket sock, const char *path)
{
	uint8_t id = FS_MSG_CHDIR;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		return -1;

	uint16_t path_len = strlen(path) + 1;
	if (ax_socket_syncsend(sock, &path_len, sizeof path_len))
		return -1;
	if (ax_socket_syncsend(sock, path, path_len))
		return -1;

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret))
		return -1;
	if (ret.error_code) {
		flea_perror(ret.error_code);
	}
	
	return 0;
}

int delete_file(ax_socket sock, const char *filename)
{
	uint8_t id = FS_MSG_DELETE;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		return -1;

	uint16_t filename_len = strlen(filename) + 1;
	if (ax_socket_syncsend(sock, &filename_len, sizeof filename_len))
		return -1;
	if (ax_socket_syncsend(sock, filename, filename_len))
		return -1;

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret))
		return -1;
	if (ret.error_code) {
		flea_perror(ret.error_code);
	}
	
	return 0;
}

int put_file(ax_socket sock, const char *filename)
{
	int retval = 0;
	FILE *fp = NULL;
	struct msg_ret_code ret = { MSG_EOK };

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "%s: failed to open file: %s\n", flea_module_name(), strerror(errno));
		goto out;
	}
	if (fseek(fp, 0, SEEK_END) == -1) {
		fprintf(stderr, "%s: %s", flea_module_name(), strerror(errno));
		goto out;
	}

	uint8_t id = FS_MSG_PUT;
	if (ax_socket_syncsend(sock, &id, sizeof id)) {
		retval = -1;
		goto out;
	}

	char path_buf[FS_MAX_PATH];
	strcpy(path_buf, filename);
	char *base_name = basename(path_buf);
	uint16_t name_len = strlen(base_name) + 1;

	if (ax_socket_syncsend(sock, &name_len, sizeof name_len)) {
		retval = -1;
		goto out;
	}
	if (ax_socket_syncsend(sock, base_name, name_len)) {
		retval = -1;
		goto out;
	}

	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		retval = -1;
		goto out;
	}
	if (ret.error_code) {
		flea_perror(ret.error_code);
		goto out;
	}

	uint32_t file_size = ftell(fp);
	if (ax_socket_syncsend(sock, &file_size, sizeof file_size)) {
		retval = -1;
		goto out;
	}
	fseek(fp, 0, SEEK_SET);
	char buf[1024];
	ssize_t read_siz;
	size_t sent = 0;
	while ((read_siz = fread(buf, 1, sizeof buf, fp)) != 0) {
		if (ax_socket_syncsend(sock, buf, read_siz)) {
			retval = -1;
			goto out;
		}
		sent += read_siz;
		printf("\r%.2f%%", (float)sent / file_size * 100);
	}
	putchar ('\n');
	fclose(fp);

	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		retval = -1;
		goto out;
	}
	if (ret.error_code) {
		flea_perror(ret.error_code);
		goto out;
	}
out:
	return retval;
}

int get_file(ax_socket sock, const char *filename)
{
	int retval = 0;
	FILE *fp = NULL;
	struct msg_ret_code ret = { MSG_EOK };

	char path_buf[FS_MAX_PATH];
	strcpy(path_buf, filename);
	char *base_name = basename(path_buf);

	fp = fopen(base_name, "wb");
	if (!fp) {
		fprintf(stderr, "%s: failed to open local file: %s\n", flea_module_name(), strerror(errno));
		goto out;
	}
	uint8_t id = FS_MSG_GET;
	if (ax_socket_syncsend(sock, &id, sizeof id)) {
		retval = -1;
		goto out;
	}

	uint16_t filename_size = strlen(filename) + 1;

	if (ax_socket_syncsend(sock, &filename_size, sizeof filename_size)) {
		retval = -1;
		goto out;
	}
	if (ax_socket_syncsend(sock, filename, filename_size)) {
		retval = -1;
		goto out;
	}

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
		goto out;
	}

	clock_t time = clock();
	bool failed = false;
	size_t recv_size = 0;
	ax_forever {
		uint16_t part_size;
		if (ax_socket_syncrecv(sock, &part_size, sizeof part_size)) {
			retval = -1;
			goto out;
		}
		if (part_size == 0)
			break;
		assert(part_size <= 256);
		char buf[256];
		if (ax_socket_syncrecv(sock, buf, part_size)) {
			retval = -1;
			goto out;
		}
		if (!failed && fwrite(buf, 1, part_size, fp) != part_size) {
			failed = true;
			fprintf(stderr, "put: %s\n", strerror(errno));
			uint8_t cancel = 1;
			if (ax_socket_syncsend(sock, &cancel, sizeof cancel))
				retval = -1;
		} else {
			recv_size += part_size;
			if ((float)(clock() - time) > 200) {
				printf("\r%.2f%%\r", (float)recv_size / file_size * 100);
				time = clock();
			}
		}
	}

	if (!failed) {
		puts("\r100.00%");
		uint8_t cancel = 0;
		if (ax_socket_syncsend(sock, &cancel, sizeof cancel)) {
			retval = -1;
			goto out;
		}
	}

	if (ax_socket_syncrecv(sock, &ret, sizeof ret)) {
		retval = -1;
		goto out;
	}
	if (ret.error_code) {
		flea_perror(ret.error_code);
		goto out;
	}
out:
	if (fp)
		fclose(fp);
	return retval;
}


int move_file(ax_socket sock, const char *old_name, const char *new_name)
{
	uint8_t id = FS_MSG_MOVE;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		return -1;

	uint16_t old_name_len = strlen(old_name) + 1;
	if (ax_socket_syncsend(sock, &old_name_len, sizeof old_name_len))
		return -1;
	if (ax_socket_syncsend(sock, old_name, old_name_len))
		return -1;

	uint16_t new_name_len = strlen(new_name) + 1;
	if (ax_socket_syncsend(sock, &new_name_len, sizeof new_name_len))
		return -1;
	if (ax_socket_syncsend(sock, new_name, new_name_len))
		return -1;

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret))
		return -1;
	if (ret.error_code) {
		flea_perror(ret.error_code);
	}
	
	return 0;
}

int get_current_dir(ax_socket sock, char *path_buf)
{
	uint8_t id = FS_MSG_PWD;
	if (ax_socket_syncsend(sock, &id, sizeof id))
		return -1;

	struct msg_ret_code ret;
	if (ax_socket_syncrecv(sock, &ret, sizeof ret))
		return -1;
	if (ret.error_code) {
		flea_perror(ret.error_code);
		return 0;
	}

	uint16_t dir_size;
	if (ax_socket_syncrecv(sock, &dir_size, sizeof dir_size))
		return -1;

	if (ax_socket_syncrecv(sock, path_buf, dir_size))
		return -1;

	path_buf[FS_MAX_PATH - 1] = '\0';
	
	return 0;
}

void usage()
{
}

static void completion_hook(char const *buf, ax_edit_completions *pCompletion)
{
	int i;
	static const char *cmd[] = {"ls", "pwd", "cd", "mv", "rm", "put", "get", "exit", NULL};

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

	ax_socket sock = flea_tunnel_open(token, 5);
	if (sock == AX_SOCKET_INVALID) {
		ax_perror("flea_tunnel_open");
		return 1;
	}
	if (get_current_dir(sock, g_current_dir)) {
		retval = -1;
		goto out;
	}

	ax_forever {
		char ps1[PATH_MAX];
		sprintf(ps1, "[%s:%d %s]# ", argv[0], token, g_current_dir);

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

		if (strcmp(prompt_argv[0], "ls") == 0) {
			char *path = NULL;
			path = prompt_argc < 2 ? "." : prompt_argv[1];
			if (list_file(sock, path)) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "pwd") == 0) {
			char path_buf[FS_MAX_PATH];
			if (get_current_dir(sock, path_buf)) {
				retval = -1;
				goto out;
			}
			puts(path_buf);
		}

		else if (strcmp(prompt_argv[0], "cd") == 0) {
			if (prompt_argc < 2) {
				fprintf(stderr, "%s: PATH is not specified.\n", prompt_argv[0]);
				continue;
			}
			if (change_dir(sock, prompt_argv[1])) {
				retval = -1;
				goto out;
			}
			if (get_current_dir(sock, g_current_dir)) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "mv") == 0) {
			if (prompt_argc < 3) {
				fprintf(stderr, "%s: SOURCE and/or DEST are not specified.\n", prompt_argv[0]);
				continue;
			}
			if (move_file(sock, prompt_argv[1], prompt_argv[2])) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "rm") == 0) {
			if (prompt_argc < 2) {
				fprintf(stderr, "%s: FILE is not specified.\n", prompt_argv[0]);
				continue;
			}
			if (delete_file(sock, prompt_argv[1])) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "put") == 0) {
			if (prompt_argc < 2) {
				fprintf(stderr, "%s: FILE is not specified.\n", prompt_argv[0]);
				continue;
			}
			if (put_file(sock, prompt_argv[1])) {
				retval = -1;
				goto out;
			}
		}

		else if (strcmp(prompt_argv[0], "get") == 0) {
			if (prompt_argc < 2) {
				fprintf(stderr, "%s: FILE is not specified.\n", prompt_argv[0]);
				continue;
			}
			if (get_file(sock, prompt_argv[1])) {
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
