#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../userprog/process.h"
#include "../lib/kernel/list.h"
#include "../lib/user/syscall.h"
#include "../userprog/syscall-init.h"
#include "../lib/stdio.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);


int main(void)
{
	put_str("I am kernel!\n");
	init_all();
	
	
	
	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");
	
	intr_enable();
	console_put_str(" main_pid:0x");
    console_put_int(sys_getpid());
    console_put_char('\n');
	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 31, k_thread_b, "argB ");
	while (1);
	return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   char* para = arg;
   void* addr = sys_malloc(63);
   console_put_str(" thread_a ,sys_malloc(63),addr is :0x");
   console_put_int((int)addr);
   console_put_char('\n');
   
   while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   char* para = arg;
   void* addr = sys_malloc(33);
   console_put_str(" thread_b ,sys_malloc(33),addr is :0x");
   console_put_int((int)addr);
   console_put_char('\n');
   
   while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
	char* name = "u_prog_a";
	printf(" I am %s, my pid: %d ", name, getpid());

	while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
	char* name = "u_prog_b";
	printf(" I am %s, my pid: %d ", name, getpid());
	while(1);
}
