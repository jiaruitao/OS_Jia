#ifndef __USERPROG_SYSCALLINIT_H
#define __USERPROG_SYSCALLINIT_H
#include "../lib/std_int.h"
void syscall_init(void);
uint32_t sys_getpid(void);
uint32_t sys_write(char* str);
#endif