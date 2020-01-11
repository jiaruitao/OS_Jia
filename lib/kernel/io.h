#ifndef __IO_H__
#define __IO_H__

#include "../std_int.h"

void outb(uint16_t port, uint8_t data);
void outsw(uint16_t port, const void* addr, uint32_t word_cnt);
uint8_t inb(uint16_t port);
void insw(uint16_t port, void* addr, uint32_t word_cnt);


#endif