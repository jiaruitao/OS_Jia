#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "memory.h"
#include "../device/timer.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/tss.h"
#include "../userprog/syscall-init.h"
#include "../device/ide.h"
#include "../fs/fs.h"

void init_all()
{
	put_str(" init all!\n");
	idt_init();
	mem_init();
	timer_init();
	thread_init();
	console_init();
	keyboard_init();
	tss_init();
	syscall_init();   // 初始化系统调用
	ide_init();
	filesys_init();
}

