#ifndef LIBFLEA_H
#define LIBFLEA_H
#include <stdint.h>
#include <ax/event.h>

ax_socket flea_tunnel_open(uint32_t token, int tunnel);

const char *flea_module_name();

const char *flea_win32_strerror(unsigned int err);

void flea_perror(unsigned int err);

#endif
