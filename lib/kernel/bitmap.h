#ifndef __BITMAP_H__
#define __BITMAP_H__
#include "../std_int.h"
#include "../../kernel/global.h"

#define BITMAP_MASK 1

struct bitmap {
	uint32_t btmp_bytes_len;			// 位图的长度（字节为单位）
	uint8_t* bits;						// 位图的位置
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);

#endif