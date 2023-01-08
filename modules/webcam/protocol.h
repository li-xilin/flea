#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#pragma pack(push, 1)

enum {
	WEBCAM_EOPENDEVICE = 60201,
	WEBCAM_ECAPTURE,
	WEBCAM_ELISTDEVICE,
};

enum fs_msg_id {
	WEBCAM_MSG_LIST_DEVICE,
	WEBCAM_MSG_CAPTURE,
	WEBCAM_MSG_STREAM_BEGIN,
	WEBCAM_MSG_STREAM_END,
};

#pragma pack(pop)

#endif
