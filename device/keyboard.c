#include "keyboard.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/io.h"
#include "../kernel/interrupt.h"

#define KBD_BUF_PORT	0X60


static void intr_keyboard_handler(void)
{
	put_char('K');
	inb(KBD_BUF_PORT);
}



void keyboard_init(void)
{
	put_str("keyboard init start!\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done!\n");
}