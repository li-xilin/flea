#include "uidpool.h"
#include <ax/queue.h>
#include <ax/log.h>

#include <stdlib.h>
#include <errno.h>

#define THRESHOLD_FOR_POP 0xFF /* 当空闲队列长度超过该值后，从队列获取UID */

struct uidpool_st
{
	ax_queue_r uid_que; /* 所有回收的UID放入该队列 */
	size_t next_uid; /* 可获取的下一个UID */
};

uidpool *uidpool_create()
{
	uidpool *self = NULL;
	ax_queue_r que = { NULL };

	self = (uidpool *)malloc(sizeof *self);
	if (!self)
		goto fail;

	que.ax_tube = __ax_queue_construct(ax_t(u32));
	if (!que.ax_tube)
		goto fail;

	self->next_uid = 1;
	self->uid_que = que;
	return self;
fail:
	free(self);
	ax_one_free(que.ax_one);
	return NULL;
}

void uidpool_destroy(uidpool *pool)
{
	if (!pool)
		return;

	ax_one_free(pool->uid_que.ax_one);
	free(pool);
}

int uidpool_get(uidpool *pool)
{
	if (ax_tube_size(pool->uid_que.ax_tube) > THRESHOLD_FOR_POP) {
		int uid = *(int *)ax_tube_prime(pool->uid_que.ax_tube);
		ax_tube_pop(pool->uid_que.ax_tube);
		return uid;
	} else {
		if (pool->next_uid == INT_MAX) {
			errno = ENOSPC;
			return -1;
		}
		return pool->next_uid++;
	}
}

void uidpool_put(uidpool *pool, int uid)
{
	if (ax_tube_push(pool->uid_que.ax_tube, &uid)) {
		ax_perror("Failed to free uid, leak accurred");
	}
}
