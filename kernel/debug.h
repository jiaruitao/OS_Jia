#ifndef __DEBUG_H__
#define __DEBUG_H__

void panic_spin(char* filename, int line, const char* func, const char* condition);

// __FILE__, __LINE__, __func__是预定义的宏，分别表示被编译的文件名、被编译文件中的行号、被编译的函数
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)


#ifdef NDEBUG
	#define ASSERT(CONDITION) ((void)0)
#else
	#define ASSERT(CONDITION) \
		if (CONDITION) {} else {	\
		PANIC(#CONDITION);	}	 
		
#endif


#endif