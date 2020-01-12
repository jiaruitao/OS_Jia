#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "memory.h"

int main(void)
{
	put_str("I am kernel!\n");
	init_all();
	intr_disable();
	
	void* addr = get_kernel_pages(3);
	put_str("\nget_kernel_page start vaddr is ");
	put_int((uint32_t)addr);
	put_char('\n');
	
	while (1);
	return 0;
}