#include "timer.h"
#include "../kernel/interrupt.h"
#include "../lib/std_int.h"
#include "../thread/thread.h"
#include "../lib/kernel/print.h"
#include "../kernel/debug.h"


uint32_t ticks;

static void intr_timer_handler(void)
{
	struct task_struct* cur_thread = running_thread();
	
	ASSERT(cur_thread->stack_magic == 0x19950209);
	
	cur_thread->elapsed_ticks++;
	ticks++;
	
	if (cur_thread->ticks == 0){
		schedule();
	}else{
		cur_thread->ticks--;
	}
}



void timer_init(void)
{
	put_str("timer init start!\n");
	register_handler(0x20, intr_timer_handler);			// 注册中断处理函数
	put_str("timer init done!\n");
}


