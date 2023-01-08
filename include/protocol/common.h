#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H
#include <stdint.h>

enum {
	MSG_EOK = 0,
};

#pragma pack(push, 1)

struct msg_ret_code
{
	uint16_t error_code;
};

#pragma pack(pop)

#endif
