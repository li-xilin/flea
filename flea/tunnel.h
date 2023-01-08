#ifndef TUNNEL_H
#define TUNNEL_H
#include <stdint.h>
#include <ax/event.h>

ax_socket tunnel_open(const char *keyword, const char *relay_host, uint16_t relay_port, uint32_t token, int tunnel);

#endif
