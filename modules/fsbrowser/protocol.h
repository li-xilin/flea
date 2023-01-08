#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#pragma pack(push, 1)

#define FS_MAX_PATH 260

enum fs_msg_id {
	FS_MSG_LIST,
	FS_MSG_CHDIR,
	FS_MSG_MOVE,
	FS_MSG_DELETE,
	FS_MSG_GET,
	FS_MSG_PUT,
	FS_MSG_STAT,
	FS_MSG_PWD,
};

struct fs_msg_file_entry {
	uint32_t file_size;
	uint8_t name_size;
	uint8_t is_dir;
	char filename[];
};
#pragma pack(pop)

#endif
