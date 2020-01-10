#include "../lib/std_int.h"
#include "../lib/kernel/print.h"

int main(void)
{
	put_char('k');
	put_char('e');
	put_char('r');
	put_char('n');
	put_char('e');
	put_char('l');
	put_char('\n');
	put_char('!');
	put_str("I am kernel!\n");
	put_int(0x123);
	put_char('\n');
	put_int(31);
	put_char('\n');
	while (1);
	return 0;
}