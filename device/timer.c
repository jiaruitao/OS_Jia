#include "timer.h"
#include "../kernel/interrupt.h"
#include "../lib/std_int.h"
#include "../thread/thread.h"
#include "../lib/kernel/print.h"
#include "../kernel/debug.h"
#include "../kernel/global.h"

#define IRQ0_FREQUENCY	   100
#define INPUT_FREQUENCY	   1193180
#define COUNTER0_VALUE	   INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT	   0x40
#define COUNTER0_NO	   0
#define COUNTER_MODE	   2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)

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

// 以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式
static void ticks_to_sleep(uint32_t sleep_ticks) 
{
	uint32_t start_tick = ticks;

	while (ticks - start_tick < sleep_ticks) {	   // 若间隔的ticks数不够便让出cpu
		thread_yield();
	}
}

// 以毫秒为单位的sleep   1秒= 1000毫秒
void mtime_sleep(uint32_t m_seconds) 
{
	uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
	ASSERT(sleep_ticks > 0);
	ticks_to_sleep(sleep_ticks); 
}

void timer_init(void)
{
	put_str("timer init start!\n");
	register_handler(0x20, intr_timer_handler);			// 注册中断处理函数
	put_str("timer init done!\n");
}


