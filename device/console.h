#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "../lib/std_int.h"

void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_char(uint8_t ch);
void console_put_int(uint32_t num);



#endif