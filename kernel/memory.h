#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "../lib/std_int.h"
#include "../lib/kernel/bitmap.h"

struct virtual_addr{
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);

#endif