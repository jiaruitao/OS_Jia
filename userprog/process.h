#ifndef __PROCESS_H__
#define __PROCESS_H__
#include "../lib/std_int.h"
#include "../thread/thread.h"

#define default_prio 31
#define USER_VADDR_START 0x8048000
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)


void start_process(void* filename_);
void page_dir_activate(struct task_struct* p_thread);
void process_activate(struct task_struct* p_thread);
uint32_t create_page_dir(void);
void process_execute(void* filename, char* name);


#endif