%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

	jmp loader_start
; 构建 GDT,及其内部描述符 ======================================= 
GDT_BASE:
	dd 0x00000000
	dd 0x00000000
CODE_DESC:
	dd 0x0000FFFF
	dd DESC_CODE_HIGH4
DATA_STACK_DESC:
	dd 0x0000FFFF
	dd DESC_DATA_HIGH4
VIDEO_DESC:
	dd 0x80000007
	dd DESC_VIDEO_HIGH4

GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1
times 60 dq 0 						; 预留 60 个描述符的空位
SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

gdt_ptr							; GDT 寄存器中要存的
	dw GDT_LIMIT
	dd GDT_BASE

; ================== loader_start ===================================
loader_start:
	mov byte [gs:0xa0], 'l'				; 低字节，放 ASCII 
	mov byte [gs:0xa1], 0x17			; 高字节放属性，蓝底白字，不闪烁
	
	mov byte [gs:0xa2], 'o'
	mov byte [gs:0xa3], 0x17
	
	mov byte [gs:0xa4], 'a'
	mov byte [gs:0xa5], 0x17
	
	mov byte [gs:0xa6], 'd'
	mov byte [gs:0xa7], 0x17
	
	mov byte [gs:0xa8], 'e'
	mov byte [gs:0xa9], 0x17
	
	mov byte [gs:0xaa], 'r'
	mov byte [gs:0xab], 0x17
	
; ================ 进入保护模式 =====================================
; 第一步：打开 A20
	in al, 0x92
	or al, 0x02
	out 0x92, al
	
; 第二步：加载 GDT
	lgdt [gdt_ptr]
	
; 第三步：cr0 的第 0 位置 1.
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

; =====================================================================
	jmp dword SELECTOR_CODE:p_mode_start		;   刷新流水线

; ============= 以下位 32 位保护模式 ==================================	
[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	
	mov byte[gs:320], 'P'
	mov byte[gs:321], 0x97
	
	jmp $

