#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include "../lib/std_int.h"

typedef void* intr_handler;

enum intr_status{
	INTR_OFF,			// 0
	INTR_ON				// 1
};


void idt_init(void);
enum intr_status intr_enable(void);
enum intr_status intr_disable(void);
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_get_status(void);
void register_handler(uint8_t vector_no, intr_handler function);

#endif