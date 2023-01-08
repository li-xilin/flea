#ifndef RSTACK_H
#define RSTACK_H

#include <stddef.h>
#include <stdbool.h>

struct rstack;

enum rstack_next_type
{
        RSTACK_NT_RESET,
        RSTACK_NT_BLOCK,
        RSTACK_NT_STREAM,
};

struct rstack_frame
{
        enum rstack_next_type type;
        const char *data;
        size_t size;
};

typedef enum rstack_next_type rstack_next_cb(struct rstack *sbuf, size_t *next_size); //并非每次都调用

struct rstack *rstack_init(rstack_next_cb *next_cb);

void rstack_reset(struct rstack *rstack);

void *rstack_get_buffer(struct rstack *rstack, size_t *buf_size);

bool rstack_notify(struct rstack *rstack, size_t size);

void rstack_commit(struct rstack *rstack, size_t size);

const struct rstack_frame *rstack_get_frame(struct rstack *rstack, int *depth);

#endif
