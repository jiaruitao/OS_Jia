#include "interrupt.h"
#include "../lib/std_int.h"
#include "global.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"

#define IDT_DESC_CNT 0x30		// 支持的中断数，每个对应一个中断描述符，存在 IDT 中

#define PIC_M_CTRL	0x20		// 主片 控制
#define PIC_M_DATA	0x21		// 主片 数据
#define PIC_S_CTRL	0xa0		// 从片 控制
#define PIC_S_DATA	0xa1		// 从片 数据

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile ("pushfl; popl %0" : "=g" (EFLAG_VAR))

extern void lodidt(uint16_t, uint32_t);


/* ******************** 初始化 8259A *********************************/
static void pic_init(void)			
{
	outb(PIC_M_CTRL, 0x11);
	outb(PIC_M_DATA, 0x20);
	
	outb(PIC_M_DATA, 0x04);
	outb(PIC_M_DATA, 0x01);
	
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0x28);
	
	outb(PIC_S_DATA, 0x02);
	outb(PIC_S_DATA, 0x01);
	
	outb(PIC_M_DATA, 0xfc);		// 时钟中断 0xfe;  键盘中断 0xfd;  时钟和键盘中断全开 0xfc 
	outb(PIC_S_DATA, 0xff);
	
	put_str("  pic init done\n");
}


struct gate_desc{
	uint16_t func_offset_low_word;
	uint16_t selector;
	uint8_t dcount;
	uint8_t attribute;
	uint16_t func_offset_high_word;
};

static struct gate_desc idt[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];
char* intr_name[IDT_DESC_CNT];		     // 用于保存异常的名字
void* idt_table[IDT_DESC_CNT];			// 用于存在 c 中的真正的中断处理函数地址



static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
	p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void)
{
	int i = 0;
	for (i = 0; i < IDT_DESC_CNT; i++){
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	put_str(" idt_desc_init\n");
}

/* 通用的中断处理函数,一般用在异常出现时的处理 */
static void general_intr_handler(uint8_t vec_nr) {
   if (vec_nr == 0x27 || vec_nr == 0x2f) {	// 0x2f是从片8259A上的最后一个irq引脚，保留
      return;		//IRQ7和IRQ15会产生伪中断(spurious interrupt),无须处理。
   }
   put_str("int vector: 0x");
   put_int(vec_nr);
   put_char('\n');
   
   while(1);
}
/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init(void) {			    // 完成一般中断处理函数注册及异常名称注册
   int i;
   for (i = 0; i < IDT_DESC_CNT; i++) {

/* idt_table数组中的函数是在进入中断后根据中断向量号调用的,
 * 见kernel/kernel.S的call [idt_table + %1*4] */
      idt_table[i] = general_intr_handler;		    // 默认为general_intr_handler。
							    // 以后会由register_handler来注册具体处理函数。
      intr_name[i] = "unknown";				    // 先统一赋值为unknown 
   }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";

}


/* 中断处理程序注册函数 */
void register_handler(uint8_t vector_no, intr_handler function)
{
	idt_table[vector_no] = function;
}



/* *************** 中断初始化 *********************************/
void idt_init(void)
{
	put_str("idt init start\n");
	idt_desc_init();					// 构造 IDT 表
	pic_init();							// 初始化 8259A
	exception_init();
//	uint64_t idt_operand = ((sizeof(idt) - 1) | (uint64_t)((uint32_t)idt << 16));
//	asm volatile ("lidt %0" : : "m" (idt_operand));			// 加载 64 位 idt 寄存器
	
	lodidt((uint16_t)(sizeof(idt) - 1), (uint32_t)(&idt));
	
	put_str("idt init done\n");
}

/* 开中断 */
enum intr_status intr_enable(void)
{
	enum intr_status old_status;
	if (INTR_ON == intr_get_status()){
		old_status = INTR_ON;
		return old_status;
	} else {
		old_status = INTR_OFF;
		asm volatile ("sti");
		return old_status;
	}
}

/* 关中断 */
enum intr_status intr_disable(void)
{
	enum intr_status old_status;
	if (INTR_ON == intr_get_status()){
		old_status = INTR_ON;
		asm volatile ("cli" : : : "memory");
		return old_status;
	} else {
		old_status = INTR_OFF;
		return old_status;
	}
}

enum intr_status intr_set_status(enum intr_status status)
{
	return status &	INTR_ON ?  intr_enable() : intr_disable();
}
/* 获取中断的状态 */
enum intr_status intr_get_status(void)
{
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}