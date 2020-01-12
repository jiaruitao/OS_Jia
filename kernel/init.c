#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "memory.h"
#include "../device/timer.h"

void init_all()
{
	put_str(" init all!\n");
	idt_init();
	mem_init();
	timer_init();
	thread_init();
	
}

