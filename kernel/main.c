#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"

void kthread_a(void* arg);
void kthread_b(void* arg);

int main(void)
{
	put_str("I am kernel!\n");
	init_all();
	
	
	thread_start("k_thread_a", 31, kthread_a, "argA ");
	thread_start("k_thread_b", 31, kthread_b, "argB ");
	
	intr_enable();
	while (1){
		put_str("Main ");
	}
	return 0;
}

void kthread_a(void* arg)
{
	char* para = arg;
	while (1){
		put_str(para);
	}
}

void kthread_b(void* arg)
{
	char* para = arg;
	while (1){
		put_str(para);
	}
}
