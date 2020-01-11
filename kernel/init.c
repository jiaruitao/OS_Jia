#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"

void init_all()
{
	put_str(" init all!\n");
	idt_init();
}

