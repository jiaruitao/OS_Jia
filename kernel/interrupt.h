#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

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

#endif