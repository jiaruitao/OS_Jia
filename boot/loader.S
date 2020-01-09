%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR

	mov byte [gs:0x10], 'L'				; 低字节，放 ASCII 
	mov byte [gs:0x11], 0x17			; 高字节放属性，蓝底红字，不闪烁
	
	mov byte [gs:0x12], 'o'
	mov byte [gs:0x13], 0x17
	
	mov byte [gs:0x14], 'd'
	mov byte [gs:0x15], 0x17
	
	mov byte [gs:0x16], 'e'
	mov byte [gs:0x17], 0x17
	
	mov byte [gs:0x18], 'r'
	mov byte [gs:0x19], 0x17
	
	jmp $