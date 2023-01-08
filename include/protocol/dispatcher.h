#ifndef PROTOCOL_DISPATCHER_H
#define PROTOCOL_DISPATCHER_H
#include <stdint.h>

#define MOD_NAME_MAX 16
#define PROTO_PASSWORD_MAX 16

enum {
	MSG_EOK = 0,
	MSG_EILTOK = 60001,
	MSG_EILTUN,
	MSG_ETUNINUSE,
	MSG_EMODINUSE,
	MSG_EWRITEF,
	MSG_EMAXMOD,
	MSG_ELDLIB,
	MSG_ENOMOD,
	MSG_ENOMEM,
};

enum {
	MSG_SVR_INSMOD,
	MSG_SVR_RMMOD,
	MSG_SVR_LSMOD,
	MSG_SVR_SYSINFO,
};

enum {
	MSG_RLY_LSSVR,
	MSG_RLY_RMSVR,
};

#pragma pack(push, 1)

struct msg_cnt_login
{
	char password[PROTO_PASSWORD_MAX];
	uint32_t token;
	uint8_t tunnel;
};

struct msg_host_info
{
	uint32_t inaddr;
	uint32_t token;
};

struct msg_host_set
{
	uint32_t count;
	struct msg_host_info host[];
};

struct msg_hello
{
	uint32_t token;
	uint8_t tunnel;
};


struct msg_reply_hello
{
	uint32_t token;
};

struct msg_insmod
{
	uint32_t size;
	char name[MOD_NAME_MAX];
	char data[];
};

struct msg_rmmod
{
	char name[MOD_NAME_MAX];
};

struct msg_mod_set
{
	uint8_t count;
	char name[][MOD_NAME_MAX];
};

struct msg_mod_error
{
	char mod_name[MOD_NAME_MAX];
	uint8_t error_code;
};

struct msg_mod_log
{
	char mod_name[MOD_NAME_MAX];
	uint16_t length;
	char text[];
};


struct msg_sysinfo
{
	char hostname[16];
	// char username[64];
	// uint32_t system_version;
	// uint32_t uptime;
	// uint8_t memory_size_gb;
	// uint8_t disk_size_gb;
	// uint8_t thread_num;
};

struct msg_ret_code
{
	uint16_t error_code;
};

#pragma pack(pop)

#endif
