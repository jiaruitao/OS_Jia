#include "thread.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/list.h"
#include "../kernel/debug.h"
#include "../userprog/process.h"
#include "sync.h"

#define PG_SIZE 4096


struct task_struct* main_thread;			// 主线程PCB
struct task_struct* idle_thread;   			 // idle线程
struct list thread_ready_list;				// 就绪队列
struct list thread_all_list;				// 所有任务队列
struct lock pid_lock;		    			// 分配pid锁
static struct list_elem* thread_tag;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/* 系统空闲时运行的线程 */
static void idle(void* arg) {
	while(1) {
		thread_block(TASK_BLOCKED);     
		//执行hlt时必须要保证目前处在开中断的情况下
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* 获取当前线程的 PCB 指针 */
struct task_struct* running_thread(void)
{
	uint32_t esp;
	asm ("mov %%esp, %0" : "=g" (esp));
	return (struct task_struct*)(esp & 0xfffff000);
}

/* 由 kernel_thread 去执行线程真正要执行的函数 */
static void kernel_thread(thread_func* function, void* func_arg)
{
	intr_enable();
	function(func_arg);
}

/* 分配pid */
static pid_t allocate_pid(void) {
   static pid_t next_pid = 0;
   lock_acquire(&pid_lock);
   next_pid++;
   lock_release(&pid_lock);
   return next_pid;
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
	pthread->pid = allocate_pid();
	strcpy(pthread->name, name);
	
	if (pthread == main_thread){
		pthread->status = TASK_RUNNING;
	}else{
		pthread->status = TASK_READY;
	}
	
//	pthread->status = TASK_RUNNING;
	pthread->priority = prio;
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);		//栈指向页的最顶处
	pthread->stack_magic = 0x19950209;	
}

struct task_struct* thread_start(char* name, int prio, thread_func* function, void* func_arg)
{
	struct task_struct* thread = get_kernel_pages(1);			//申请一页作为PCB空间
	init_thread(thread, name, prio);							// 初始化PCB
	thread_creat(thread, function, func_arg);					// 初始化线程栈
	
	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);		// 加到就绪队列中
	
	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);
	
//	asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");
	return thread;
}

void schedule()
{
	ASSERT(intr_get_status() == INTR_OFF);
	
	struct task_struct* cur = running_thread();
	if (cur->status == TASK_RUNNING){
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	}else{
		
	}
	
	/* 如果就绪队列中没有可运行的任务,就唤醒idle */
	if (list_empty(&thread_ready_list)) {
		thread_unblock(idle_thread);
	}
   
	ASSERT(!list_empty(&thread_ready_list));
	thread_tag = NULL;
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag); 
	next->status = TASK_RUNNING;
	
	process_activate(next);			// 激活任务页表，next若是用户进程则更新tss中的esp0
	
	switch_to(cur, next);
}

/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(enum task_status stat) {
/* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
   ASSERT(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
   enum intr_status old_status = intr_disable();
   struct task_struct* cur_thread = running_thread();
   cur_thread->status = stat; // 置其状态为stat 
   schedule();		      // 将当前线程换下处理器
/* 待当前线程被解除阻塞后才继续运行下面的intr_set_status */
   intr_set_status(old_status);
}

/* 将线程pthread解除阻塞 */
void thread_unblock(struct task_struct* pthread) {
   enum intr_status old_status = intr_disable();
   ASSERT(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
   if (pthread->status != TASK_READY) {
      ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
      if (elem_find(&thread_ready_list, &pthread->general_tag)) {
	 PANIC("thread_unblock: blocked thread in ready_list\n");
      }
      list_push(&thread_ready_list, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
      pthread->status = TASK_READY;
   } 
   intr_set_status(old_status);
}

/* 主动让出cpu,换其它线程运行 */
void thread_yield(void) {
	struct task_struct* cur = running_thread();   
	enum intr_status old_status = intr_disable();
	ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
	list_append(&thread_ready_list, &cur->general_tag);
	cur->status = TASK_READY;
	schedule();
	intr_set_status(old_status);
}

static void make_main_thread(void)
{
	main_thread = running_thread();
	init_thread(main_thread, "main", 31);
	
	ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag);
}

void thread_init(void)
{
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	lock_init(&pid_lock);
	make_main_thread();
	/* 创建idle线程 */
	idle_thread = thread_start("idle", 10, idle, NULL);
	put_str("thread_init done!\n");
}

