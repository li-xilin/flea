#ifndef PREBOOT_H
#define PREBOOT_H
#include <ax/reactor.h>
#include <ax/list.h>
enum preboot_state
{
	PREBOOT_STATE_FILTER,
	PREBOOT_STATE_PRETEND,
	PREBOOT_STATE_BOOT,
};

struct preboot_host_sess
{
        uint32_t src_addr;
        uint32_t dst_addr;
        uint32_t query_count;
        time_t last_time;
        enum preboot_state state;
        uint32_t serial_hash;
        uint8_t major_version;
        uint8_t minor_version;
        uint8_t flags;
        char hostname[9];
};

int preboot_startup(ax_reactor *reactor);

void preboot_cleanup();

ax_trait_declare(preboot_host_sess, struct preboot_host_sess);

int preboot_session_list(ax_list_r session_list);

void preboot_set_stranger_state(enum preboot_state state);

int preboot_set_state(uint64_t host_uuid, enum preboot_state state);

int preboot_set_bootloader_code(ax_byte *payload, size_t size);

int preboot_set_server_code(ax_byte *payload, size_t size);

#endif
