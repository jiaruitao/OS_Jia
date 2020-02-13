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
	void* addr1 = sys_malloc(256);
	void* addr2 = sys_malloc(255);
	void* addr3 = sys_malloc(254);
	console_put_str(" thread_a malloc addr:0x");
	console_put_int((int)addr1);
	console_put_char(',');
	console_put_int((int)addr2);
	console_put_char(',');
	console_put_int((int)addr3);
	console_put_char('\n');

	int cpu_delay = 100000;
	while(cpu_delay-- > 0);
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
	void* addr1 = sys_malloc(256);
	void* addr2 = sys_malloc(255);
	void* addr3 = sys_malloc(254);
	console_put_str(" thread_b malloc addr:0x");
	console_put_int((int)addr1);
	console_put_char(',');
	console_put_int((int)addr2);
	console_put_char(',');
	console_put_int((int)addr3);
	console_put_char('\n');

	int cpu_delay = 100000;
	while(cpu_delay-- > 0);
	sys_free(addr1);
	sys_free(addr2);
	sys_free(addr3);
	while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);
	printf(" prog_a malloc addr:0x%x,0x%x,0x%x ", (int)addr1, (int)addr2, (int)addr3);

	//	printf(" prog_a malloc addr:0x%x 0x%x ", (int)addr1, (int)addr2);
	int cpu_delay = 100000;
	while(cpu_delay-- > 0);
	free(addr1);
	free(addr2);
	free(addr3);

	while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
	void* addr1 = malloc(256);
	void* addr2 = malloc(255);
	void* addr3 = malloc(254);
	printf(" prog_b malloc addr:0x%x,0x%x,0x%x ", (int)addr1, (int)addr2, (int)addr3);

	//	printf(" prog_a malloc addr:0x%x 0x%x ", (int)addr1, (int)addr2);
	int cpu_delay = 100000;
	while(cpu_delay-- > 0);
	free(addr1);
	free(addr2);
	free(addr3);
	while(1);
}
