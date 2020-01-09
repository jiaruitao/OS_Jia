;主引导程序
;----------------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00		;告诉编译器，起始地址编译为0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00				;栈指向0x7c00
	mov ax, 0xb800				;实模式中的显存分布中，文本模式显存地址为 0xb8000
	mov gs, ax

;清屏利用 0x06 号功能，上卷全部行，则可清屏
;------------------------------------------
; INT 0X10		功能号：0x06 	功能描述：上卷窗口
;------------------------------------------
; 输入: 
; AH 功能号 = 0x06
; AL = 上卷的行数，如果为 0 表示全部
; BH = 上卷行的属性
; (CL, CH) = 窗口左上角的(X,Y)的位置
; (DL, DH) = 窗口右下角的(X,Y)的位置

	mov ax, 0x600		; 功能号：0x06， 全部上卷
	mov bx, 0x700
	mov cx, 0			; 左上角 (0,0)		窗口大小为 25*80
	mov dx, 0x184f		; 右下角 (24,79)

	int 0x10					; 利用 BIOS 的 0x10 中断

; 直接往显存放入数据，一个字符占 2 个字节 =====================================
; 低字节放 ASCII 码，高字节放字符属性，如前景色、背景色、亮度、是否闪烁 =======
	mov byte [gs:0x00], 'O'				; 低字节，放 ASCII 
	mov byte [gs:0x01], 0x14			; 高字节放属性，蓝底红字，不闪烁
	
	mov byte [gs:0x02], 'S'
	mov byte [gs:0x03], 0x14
	
	mov byte [gs:0x04], '_'
	mov byte [gs:0x05], 0x14
	
	mov byte [gs:0x06], 'J'
	mov byte [gs:0x07], 0x14
	
	mov byte [gs:0x08], 'i'
	mov byte [gs:0x09], 0x14
	
	mov byte [gs:0x0a], 'a'
	mov byte [gs:0x0b], 0x14

; 硬盘读取函数的调用 ==========================================================
	mov eax, LOADER_START_SECTOR	; 起始扇区 LBA 地址
	mov bx, LOADER_BASE_ADDR		; 写入的地址
	mov cx, 1						; 读 1 个扇区
	call rd_disk_m_16
	
	jmp LOADER_BASE_ADDR			; 跳到 loader.S 执行

; 硬盘读取函数 ================================================================
rd_disk_m_16:
	mov esi, eax					; 备份 eax : 起始扇区 LBA 地址
	mov di, cx						; 备份 cx :读取的扇区数

; 第一步：设置要读取的扇区数
	mov dx, 0x1f2
	mov al, cl
	out dx, al
	
	mov eax, esi
; 第二步：往3个LBA寄存器中写入扇区起始地址的低24位
	mov dx, 0x1f3
	out dx, al
	
	mov dx, 0x1f4
	mov cl, 8
	shr eax, cl
	out dx, al
	
	mov dx, 0x1f5
	shr eax, cl
	out dx, al

; 第三步：写入 device 寄存器
	mov dx, 0x1f6
	shr eax, cl
	and al, 0x0f
	or al, 0xe0
	out dx, al
	
; 第四步：往 command 寄存器写入操作命令
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

; 第五步：读取status寄存器看是否工作完成
.not_ready:
	nop
	in al, dx
	and al, 0x88
	cmp al, 0x08
	jnz .not_ready

;第六步；从 0x10f 读取数据
	mov ax, di
	mov dx, 256						; 每次读一个字，每个扇区512字节，每个扇区读256次
	mul dx							; 被乘数 AX, 乘数 DX， 乘积放在 DX:AX
	mov cx, ax						; 乘积放入CX作为循环次数

	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [bx], ax
	add bx, 2
	loop .go_on_read
	ret

; =============================================================================

	times 510-($ - $$) db 0	; 从当前行一直到 510 字节处填充 0 
	db 0x55, 0xaa		; 魔数
	
	