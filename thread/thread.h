#ifndef __THREAD_H__
#define __THREAD_H__

#include "../lib/std_int.h"
#include "../lib/kernel/list.h"




typedef void thread_func(void*);

enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

/* **** 中断栈 ******** */
struct intr_stack{
	uint32_t vec_no;		// 后入栈, 低地址
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	
	uint32_t err_code;
	void (*eip) (void);
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss;		// 最先入栈, 高地址处
};

/* ***** 线程栈 ******* */
struct thread_stack{
	uint32_t ebp;			//低地址
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	void (*eip)(thread_func* func, void* func_arg);
	void (*unused_retaddr);
	thread_func* function;
	void* func_arg;
};

struct task_struct{
	uint32_t* self_kstack;
	enum task_status status;
	uint8_t priority;			// 线程优先级
	char name[16];				// 线程名字
	uint8_t ticks;
	uint32_t elapsed_ticks;
	struct list_elem general_tag;
	struct list_elem all_list_tag;
	uint32_t* pgdir;
	uint32_t stack_magic;		// 栈的标记，检测溢出
};

struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg);
struct task_struct* running_thread(void);
void schedule(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);


#endif