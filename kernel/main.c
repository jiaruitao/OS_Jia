#include "../lib/std_int.h"
#include "../lib/kernel/print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"

int main(void)
{
	put_str("I am kernel!\n");
	init_all();
	intr_enable();
	
	ASSERT(1 == 2);
	
	while (1);
	return 0;
}