#include "sync.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"

/* 信号量的初始化 */
void sema_init(struct semaphore* psema, uint8_t value)
{
	psema->value = value;
	list_init(&psema->waiters);
}

/* 锁的初始化 */
void lock_init(struct lock* plock)
{
	plock->holder = NULL;
	sema_init(&plock->semaphore, 1);			//信号量初始值为 1	
	plock->holder_repeat_nr = 0;
}

/* 信号量的 down 操作 （p 操作）*/
void sema_down(struct semaphore* psema)
{
	enum intr_status old_status = intr_disable();
	while (psema->value == 0) {
		ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
		if (elem_find(&psema->waiters, &running_thread()->general_tag)){
			ASSERT(1 == 2);
		}
		list_append(&psema->waiters, &running_thread()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	psema->value--;
	ASSERT(psema->value == 0);
	
	intr_set_status(old_status);
}

/* 信号量的 up 操作 （v 操作）*/
void sema_up(struct semaphore* psema)
{
	enum intr_status old_status = intr_disable();
	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)) {
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;
	ASSERT(psema->value == 1);
	
	intr_set_status(old_status);
}

/* 获取锁 plock */
void lock_acquire(struct lock* plock) {
/* 排除曾经自己已经持有锁但还未将其释放的情况。*/
   if (plock->holder != running_thread()) { 
      sema_down(&plock->semaphore);    // 对信号量P操作,原子操作
      plock->holder = running_thread();
      ASSERT(plock->holder_repeat_nr == 0);
      plock->holder_repeat_nr = 1;
   } else {
      plock->holder_repeat_nr++;
   }
}

/* 释放锁 plock */
void lock_release(struct lock* plock) {
   ASSERT(plock->holder == running_thread());
   if (plock->holder_repeat_nr > 1) {
      plock->holder_repeat_nr--;
      return;
   }
   ASSERT(plock->holder_repeat_nr == 1);

   plock->holder = NULL;	   // 把锁的持有者置空放在V操作之前
   plock->holder_repeat_nr = 0;
   sema_up(&plock->semaphore);	   // 信号量的V操作,也是原子操作
}

