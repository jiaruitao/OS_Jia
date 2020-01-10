TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
section .data
put_int_buffer dq 0

section .text

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
	mov ax, bx
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
; ====================== 打印字符串 ===================================	
[bits 32]
global put_str
put_str:
	push ebx
	push ecx
	xor ecx, ecx
	mov ebx, [esp + 12]			; 取得字符串的地址
.goon:
	mov cl, [ebx]
	cmp cl, 0
	jz .str_over
	push ecx
	call put_char
	add esp, 4
	inc ebx
	jmp .goon
	
.str_over:
	pop ecx
	pop ebx
	ret
	
; ==================== 打印整数 =========================================
[bits 32]
global put_int
put_int:
	pushad 
	mov ebp, esp
	mov eax, [ebp + 36]
	mov edx, eax
	mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
	mov ecx, 8			       ; 32位数字中,16进制数字的位数是8个
	mov ebx, put_int_buffer

	;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数字
.16based_4bits:			       ; 每4位二进制是16进制数字的1位,遍历每一位16进制数字
	and edx, 0x0000000F		       ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
	cmp edx, 9			       ; 数字0～9和a~f需要分别处理成对应的字符
	jg .is_A2F 
	add edx, '0'			       ; ascii码是8位大小。add求和操作后,edx低8位有效。
	jmp .store
.is_A2F:
	sub edx, 10			       ; A~F 减去10 所得到的差,再加上字符A的ascii码,便是A~F对应的ascii码
	add edx, 'A'

	;将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
	;高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
	; 此时dl中应该是数字对应的字符的ascii码
	mov [ebx+edi], dl		       
	dec edi
	shr eax, 4
	mov edx, eax 
	loop .16based_4bits

	;现在put_int_buffer中已全是字符,打印之前,
	;把高位连续的字符去掉,比如把字符000123变成123
.ready_to_print:
	inc edi			       ; 此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:  
	cmp edi,8			       ; 若已经比较第9个字符了，表示待打印的字符串为全0 
	je .full0 
	;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
	mov cl, [put_int_buffer+edi]
	inc edi
	cmp cl, '0' 
	je .skip_prefix_0		       ; 继续判断下一位字符是否为字符0(不是数字0)
	dec edi			       ;edi在上面的inc操作中指向了下一个字符,若当前字符不为'0',要恢复edi指向当前字符		       
	jmp .put_each_num

.full0:
	mov cl,'0'			       ; 输入的数字为全0时，则只打印0
.put_each_num:
	push ecx			       ; 此时cl中为可打印的字符
	call put_char
	add esp, 4
	inc edi			       ; 使edi指向下一个字符
	mov cl, [put_int_buffer+edi]	       ; 获取下一个字符到cl寄存器
	cmp edi,8
	jl .put_each_num
	popad
	ret

