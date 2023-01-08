#include "rstack.h"
#include <stdlib.h>
#include <assert.h>

#define STREAM_BUF_SIZE 1024
#define DEPTH_MAX 8

struct rstack
{
	rstack_next_cb *next_cb;

	int depth;
	struct rstack_frame frame[DEPTH_MAX];

	size_t frame_size;
	enum rstack_next_type next_type;

	char *blk_buf;
	size_t blk_buf_size;
	size_t blk_buf_offset;

	char stm_buf[STREAM_BUF_SIZE];
	size_t stm_buf_offset;

	bool commited;
};

void rstack_reset(struct rstack *rstack)
{
	rstack->depth = 0;
	rstack->frame[0].type = rstack->next_cb(rstack, &rstack->frame_size);
	rstack->depth = 1;
	rstack->frame[0].size = 0;
	rstack->frame[0].data = rstack->blk_buf;

	rstack->blk_buf_offset = 0;
	rstack->stm_buf_offset = 0;

	rstack->commited = false;
}



struct rstack *rstack_init(rstack_next_cb *next_cb)
{
	struct rstack *stack = malloc(sizeof *stack);
	if (!stack)
		return NULL;

	stack->blk_buf_size = 1024; //TODO

	stack->blk_buf = malloc(stack->blk_buf_size);
	if (!stack->blk_buf) {
		free(stack);
		return NULL;
	}

	stack->next_cb = next_cb;

	rstack_reset(stack);
	return stack;
}


void *rstack_get_buffer(struct rstack *rstack, size_t *buf_size)
{
	struct rstack_frame *frame = &rstack->frame[rstack->depth - 1];
	if (rstack->commited) {

		if (frame->type == RSTACK_NT_RESET) {
			rstack->depth = 0;
			rstack_reset(rstack);
			frame = rstack->frame;
		}
		else if (frame->type == RSTACK_NT_STREAM && frame->size == rstack->frame_size || frame->type == RSTACK_NT_BLOCK) {
			if (rstack->frame[rstack->depth - 1].type == STREAM_BUF_SIZE) {
				rstack->stm_buf_offset = 0;
				rstack->frame[rstack->depth - 1].data = NULL;
			}

			enum rstack_next_type next_type = rstack->next_cb(rstack, &rstack->frame_size);
			assert(rstack->depth < DEPTH_MAX);
			rstack->frame[rstack->depth].data = 
				(next_type == RSTACK_NT_BLOCK
				? rstack->blk_buf + rstack->blk_buf_offset
				: rstack->stm_buf);

			rstack->frame[rstack->depth].size = 0;
			rstack->frame[rstack->depth].type = next_type;
			rstack->depth ++;
			frame += 1;
		}
		rstack->commited = false;
	}


	if (frame->type == RSTACK_NT_RESET) {
		*buf_size = 0;
		return rstack->blk_buf;
	}
	if (frame->type == RSTACK_NT_BLOCK) {
		*buf_size = rstack->frame_size - frame->size;
		return rstack->blk_buf + rstack->blk_buf_offset;
	}
	if (frame->type == RSTACK_NT_STREAM) {
		size_t left_size = rstack->frame_size - frame->size;

		if (left_size > STREAM_BUF_SIZE - rstack->stm_buf_offset)
			left_size = STREAM_BUF_SIZE - rstack->stm_buf_offset;
		*buf_size = rstack->frame_size - frame->size;
		return rstack->stm_buf + rstack->stm_buf_offset;
	}
	return NULL;
}

bool rstack_notify(struct rstack *rstack, size_t size)
{
	assert(rstack->commited == false);
	struct rstack_frame *frame = &rstack->frame[rstack->depth - 1];

	if (frame->type == RSTACK_NT_RESET)
		return true;

	frame->size += size;

	if (frame->type == RSTACK_NT_STREAM) {
		rstack->stm_buf_offset += size;
		assert(rstack->stm_buf_offset <= STREAM_BUF_SIZE);
		if (rstack->stm_buf_offset == STREAM_BUF_SIZE)
			rstack->stm_buf_offset = 0;
	}

	if (frame->type == RSTACK_NT_BLOCK) {
		rstack->blk_buf_offset += size;
		assert(rstack->blk_buf_offset <= rstack->blk_buf_size);
		if (frame->size < rstack->frame_size)
			return false;
	}

	return true;
}

void rstack_commit(struct rstack *rstack, size_t size)
{
	struct rstack_frame *frame = &rstack->frame[rstack->depth - 1];
	assert(rstack->commited == false);
	if (frame->type == RSTACK_NT_STREAM) {
		frame->data = rstack->stm_buf + size;
	}
	rstack->commited = true;
}

const struct rstack_frame *rstack_get_frame(struct rstack *rstack, int *depth)
{
	*depth = rstack->depth;
	return rstack->frame;
}

