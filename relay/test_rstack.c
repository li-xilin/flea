#include "rstack.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

enum rstack_next_type next_cb(struct rstack *rst, size_t *size)
{
	int depth;
	const struct rstack_frame *frame = rstack_get_frame(rst, &depth);

	enum rstack_next_type next_type;

	switch (depth) {
		case 0: {
			*size = 2;
			next_type = RSTACK_NT_BLOCK;
			break;
		}
		case 1: {
			*size = *(uint16_t *)frame[0].data;
			next_type = RSTACK_NT_STREAM;
			break;
		}
		case 2: {
			*size = 4;
			next_type = RSTACK_NT_BLOCK;
			break;
		}
		case 3: {
			*size = 0;
			next_type = RSTACK_NT_RESET;
			break;
		}
	}
	return next_type;
}

int main() {
	struct rstack *rst = rstack_init(next_cb);

	size_t size;
	char *buf;

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 2);

		buf[0] = 11;

		assert(rstack_notify(rst, 1) == 0);
	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 1);
		buf[0] = 0;
		assert(rstack_notify(rst, 1) == 1);


		int depth;
		const struct rstack_frame *frame = rstack_get_frame(rst, &depth);
		assert(depth == 1);
		assert(*(uint16_t*)frame[0].data == 11);

		rstack_commit(rst, 0);
	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 11);
		strcpy(buf, "01234");
		assert(rstack_notify(rst, 5) == 1);

		int depth;
		const struct rstack_frame *frame = rstack_get_frame(rst, &depth);
		assert(depth == 2);
		assert(strncmp(frame[1].data, "01234", 5) == 0);

		rstack_commit(rst, 5);
	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 6);
		strcpy(buf, "56789");
		assert(rstack_notify(rst, 6) == 1);

		int depth;
		const struct rstack_frame *frame = rstack_get_frame(rst, &depth);
		assert(depth == 2);
		assert(frame[1].type == RSTACK_NT_STREAM);
		assert(strncmp(frame[1].data, "56789", 6) == 0);

		rstack_commit(rst, 6);

	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 4);
		*(uint64_t *)buf = 42;
		assert(rstack_notify(rst, 4) == 1);

		int depth;
		const struct rstack_frame *frame = rstack_get_frame(rst, &depth);
		assert(depth == 3);
		assert(frame[2].type == RSTACK_NT_BLOCK);
		assert(*(uint64_t *)frame[2].data == 42);

		rstack_commit(rst, 0);

	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 0);
		assert(rstack_notify(rst, 0) == 1);

		int depth;
		const struct rstack_frame *frame = rstack_get_frame(rst, &depth);
		assert(depth == 4);
		assert(frame[3].type == RSTACK_NT_RESET);
		assert(frame[3].size == 0);

		rstack_commit(rst, 0);
	}

	{
		buf = rstack_get_buffer(rst, &size);
		assert(size == 2);

	}

}
