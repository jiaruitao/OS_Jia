#include "process.h"
#include "../kernel/memory.h"
#include "../thread/thread.h"
#include "../lib/std_int.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/list.h"
#include "../kernel/global.h"
#include "../device/console.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#include "tss.h"
#include "../lib/kernel/bitmap.h"


extern void intr_exit(void);


/* 构建进程的上下文信息 */
void start_process(void* filename_)
{
	void* function = filename_;
	struct task_struct* cur = running_thread();
	cur->self_kstack += sizeof(struct thread_stack);
	struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
	proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
	proc_stack->ebx= proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
	proc_stack->gs = 0;		// 用户态用不上显存,直接为 0
	proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;		//选择子：选用户数据段
	proc_stack->eip = function;
	proc_stack->cs = SELECTOR_U_CODE;				// 选择子： 选用户代码段
	proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
	proc_stack->ss = SELECTOR_U_DATA;
	
	asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory");	
}

/* 激活页表 */
void page_dir_activate(struct task_struct* pthread)
{
	uint32_t pagedir_phy_addr = 0x100000; 		// 内核线程的页目录表的物理地址是 0x100000
	
	if (pthread->pgdir != NULL) {		// 判断是用户进程还是内核线程
		// 是用户进程
		pagedir_phy_addr = addr_v2p((uint32_t)pthread->pgdir);
	}
	
	asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

/* 激活线程或进程的页表，更新tss中的esp0为进程的特权级为0 的栈 */
void process_activate(struct task_struct* p_thread)
{
	ASSERT(p_thread);
	page_dir_activate(p_thread);
	
	if (p_thread->pgdir){
		// 是用户进程, 需要更新 esp0
		update_tss_esp(p_thread);
	}
}


uint32_t create_page_dir(void)
{
	uint32_t* page_dir_vaddr = get_kernel_pages(1);			// 内核空间申请一个页,作为用户进程的页目录表
	if (page_dir_vaddr == NULL) {
		console_put_str("create_page_dir failed!");
	}
	
	/* ============ 1 先复制页表 ============= */
	
	// 把页目录项的768项至1024项复制过来
	memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300 * 4),(uint32_t*)(0xfffff000 + 0x300 * 4), 1024);
		
	/* ========== 2 更新页目录地址 ============= */
	uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
	page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
	
	return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog)
{
	user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
	
	//虚拟地址位图所占的页数
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
	
	user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
	user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
	
	bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);	
}


/* 创建用户进程 */
void process_execute(void* filename, char* name)
{
	struct task_struct* thread = get_kernel_pages(1);		// 申请 1 页做 进程PCB
	init_thread(thread, name, default_prio);				// 初始化PCB		
	create_user_vaddr_bitmap(thread);
	thread_creat(thread, start_process, filename);			// 初始化线程栈
	thread->pgdir = create_page_dir();						// 创建页目录表

	
	enum intr_status old_status = intr_disable();
	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);		// 加到就绪队列中
	
	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);
	intr_set_status(old_status);
	console_put_str("process_execute done!\n");	
}


