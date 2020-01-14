#ifndef __TSS_H__
#define __TSS_H__

#include "../lib/std_int.h"
#include "thread.h"


void tss_init(void);
void update_tss_esp(struct task_struct* pthread);

#endif