#ifndef PROTOCOL_PREBOOT_H
#define PROTOCOL_PREBOOT_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define PREBOOT_REQUEST_DATA_SIZE 32
#define PREBOOT_BOOTLOADER_SIGNATURE 0x000004E8

#define PREBOOT_MARK_REQUEST 0xEA
#define PREBOOT_MARK_REPLY_BOOT 0xEB

#pragma pack(push,1)

struct preboot_hdr {
	uint8_t random;
	uint8_t mark;
};
struct preboot_request {
	struct preboot_hdr hdr;
	uint8_t version;
	uint8_t flags;
	uint32_t serial_hash;
	char hostname[8];
}; 

struct preboot_reply {
	struct preboot_hdr hdr;
	uint16_t payload_size;
	char reserved[12];
}; 

#pragma pack(pop)

static void preboot_decode(const uint8_t *src, size_t src_size, struct preboot_hdr *dest)
{
	assert(src_size % 2 == 0);
	const uint8_t * p = src;
	uint8_t random = ((p[0]-'a') << 4) | (((p[1])) -'k');
	for (int i = 1; i < src_size / 2; i++) {
		((uint8_t*)dest)[i] = ((((p[i*2] - 'a') << 4) | ((p[i * 2 + 1] - 'k'))) ^ random);
	}
	dest->random = random;
}

static void preboot_encode(const struct preboot_hdr *src, size_t src_size, uint8_t *dest)
{
	for (int i = 0; i < src_size; i++) {
		uint8_t byte = ((uint8_t *)src)[i];
		byte = (i == 0) ? byte : (byte ^ src->random);
		dest[i * 2] = 'a' + (byte >> 4);
		dest[i * 2 + 1] = 'k' + (byte & 0x0F);
	}
}

#endif
