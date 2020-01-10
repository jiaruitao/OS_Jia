TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
SECTION .text

; ============== 函数 put_char ======================
global put_char
put_char:
	pushad			; 备份32位寄存器环境	8个32位寄存器
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	
;;; 获取光标位置 ;;;;;;;
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	in al, dx				; 获取光标位置的高8位
	mov ah, al
	
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx				; 获取光标位置的低8位

	mov bx, ax				; 光标位置存在 bx 寄存器中
	mov ecx, [esp + 36]		; puahad 压入 4*8=32，加上主调函数的返回地址，故esp+36
	
	cmp cl, 0xd
	jz .is_carriage_return	; 回车
	cmp cl, 0xa
	jz .is_line_feed		; 换行
	
	cmp cl, 0x8
	jz .is_backspace		; 退格
	
	jmp .put_other
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.is_backspace:
	dec bx
	shl bx, 1
	
	mov byte [gs: bx], 0x20
	inc bx
	mov byte [gs: bx], 0x07
	shr bx, 1
	jmp .set_cursor
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.put_other:
	shl bx, 1
	
	mov byte [gs: bx], cl
	inc bx
	mov byte [gs: bx], 0x07			;属性：黑底白字
	shr bx, 1
	inc bx
	cmp bx, 2000
	jl .set_cursor					; 未写到 2000, 去设置新光标
	
.is_line_feed:
.is_carriage_return:
	xor dx, dx
	mov ax, dx
	mov si, 80
	div si
	sub bx, dx

.is_carriage_return_end:
	add bx, 80
	cmp bx, 2000

.is_line_feed_end:
	jl .set_cursor


.roll_screen:
	cld
	mov ecx, 960
	
	mov esi, 0xc00b80a0
	mov edi, 0xc00b8000
	rep movsd
	
	mov ebx, 3840
	mov ecx, 80
	
.cls:
	mov word [gs:ebx], 0x0720
	add ebx, 2
	loop .cls						; 最后一行填充 0
	mov bx, 1920
	
.set_cursor:
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al 
	
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al

.put_char_done:
	popad
	ret
	
	
	