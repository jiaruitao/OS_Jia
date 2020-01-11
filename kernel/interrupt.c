#include "interrupt.h"
#include "../lib/std_int.h"
#include "global.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"

#define IDT_DESC_CNT 0x21

#define PIC_M_CTRL	0x20		// 主片 控制
#define PIC_M_DATA	0x21		// 主片 数据
#define PIC_S_CTRL	0xa0		// 从片 控制
#define PIC_S_DATA	0xa1		// 从片 数据

extern void lodidt(uint16_t, uint32_t);
// 初始化 8259A
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
	
	outb(PIC_M_DATA, 0xfe);
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



void idt_init(void)
{
	put_str("idt init start\n");
	idt_desc_init();					// 构造 IDT 表
	pic_init();							// 初始化 8259A
	
//	uint64_t idt_operand = ((sizeof(idt) - 1) | (uint64_t)((uint32_t)idt << 16));
//	asm volatile ("lidt %0" : : "m" (idt_operand));			// 加载 64 位 idt 寄存器
	
	lodidt((uint16_t)(sizeof(idt) - 1), (uint32_t)(&idt));
	
	put_str("idt init done\n");
}
