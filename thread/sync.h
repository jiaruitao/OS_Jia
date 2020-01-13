#ifndef __SYNC_H__
#define __SYNC_H__
#include "../lib/std_int.h"
#include "../lib/kernel/list.h"
#include "thread.h"

/* 用信号量实现锁机制 */
/* 信号量结构 */
struct semaphore{
	uint8_t value;
	struct list waiters;
};

/* 锁结构 */
struct lock{
	struct task_struct* holder;			// 锁的持有者
	struct semaphore semaphore;			// 用二元信号量实现锁
	uint32_t holder_repeat_nr;			// 锁的持有者反复申请锁的次数
};

void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);


#endif