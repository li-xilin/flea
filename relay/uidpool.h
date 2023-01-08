#ifndef UIDPOOL_H
#define UIDPOOL_H
#include <stddef.h>

typedef struct uidpool_st uidpool;

uidpool *uidpool_create();

void uidpool_destroy(uidpool *);

/*
 * @brief 获取从1开始的唯一ID
 */
int uidpool_get(uidpool *);

void uidpool_put(uidpool *pool, int uid);

#endif
