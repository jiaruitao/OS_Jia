#include "thread.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"

#define PG_SIZE 4096

static void kernel_thread(thread_func* function, void* func_arg)
{
	function(func_arg);
}

/* 初始化线程栈 thread_stack */
void thread_creat(struct task_struct* pthread, thread_func* function, void* func_arg)
{
	pthread->self_kstack -= sizeof(struct intr_stack);
	pthread->self_kstack -= sizeof(struct thread_stack);
	struct thread_stack* kthread_stack = (struct task_struct*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;
	kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化PCB结构 */
void init_thread(struct task_struct* pthread, char* name, int prio)
{
	memset(pthread, 0, PG_SIZE);				// 把 PCB 所在的页清 0
	strcpy(pthread->name, name);
	pthread->status = TASK_RUNNING;
	pthread->priority = prio;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);		//栈指向页的最顶处
	pthread->stack_magic = 0x19950209;	
}


struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg)
{
	struct task_struct* thread = get_kernel_pages(1);			//申请一页作为PCB空间
	init_thread(thread, name, prio);							// 初始化PCB
	thread_creat(thread, function, func_arg);					// 初始化线程栈
	
	
	asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");
	return thread;
}

