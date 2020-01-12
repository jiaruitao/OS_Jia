#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"

void kthread_a(void* arg);

int main(void)
{
	put_str("I am kernel!\n");
	init_all();
	intr_disable();
	
	thread_start("k_thread_a", 31, kthread_a, "argA ");
	
	
	while (1);
	return 0;
}

void kthread_a(void* arg)
{
	char* para = arg;
	while (1){
		put_str(para);
	}
}
