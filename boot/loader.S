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

total_mem_bytes dd 0			; total_mem_bytes 的地址为(3 + 32 + 480 + 0x900 = 0xb03),将来内存管理时要用这个地址里的值

SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

gdt_ptr	dw GDT_LIMIT
		dd GDT_BASE			; GDT 寄存器中要存的

ards_buf times 150 db 0
ards_nr dw 0		      		;用于记录ards结构体数量


; ================== loader_start ===================================
loader_start:

; ================== 检测内存容量 ===================================
;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------

   xor ebx, ebx		      ;第一次调用时，ebx值要为0
   mov edx, 0x534d4150	      ;edx只赋值一次，循环体中不会改变
   mov di, ards_buf	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
   mov eax, 0x0000e820	      ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
   mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
   int 0x15
   jc .e820_failed_so_try_e801   ;若cf位为1则有错误发生，尝试0xe801子功能
   add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
   inc word [ards_nr]	      ;记录ARDS数量
   cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
   jnz .e820_mem_get_loop

;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
   mov cx, [ards_nr]	      ;遍历每一个ARDS结构体,循环次数是ARDS的数量
   mov ebx, ards_buf 
   xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:	      ;无须判断type是否为1,最大的内存块一定是可被使用
   mov eax, [ebx]	      ;base_add_low
   add eax, [ebx+8]	      ;length_low
   add ebx, 20		      ;指向缓冲区中下一个ARDS结构
   cmp edx, eax		      ;冒泡排序，找出最大,edx寄存器始终是最大的内存容量
   jge .next_ards
   mov edx, eax		      ;edx为总内存大小
.next_ards:
   loop .find_max_mem_area
   jmp .mem_get_ok

;------  int 15h ax = E801h 获取内存大小,最大支持4G  ------
; 返回后, ax cx 值一样,以KB为单位,bx dx值一样,以64KB为单位
; 在ax和cx寄存器中为低16M,在bx和dx寄存器中为16MB到4G。
.e820_failed_so_try_e801:
   mov ax,0xe801
   int 0x15
   jc .e801_failed_so_try88   ;若当前e801方法失败,就尝试0x88方法

;1 先算出低15M的内存,ax和cx中是以KB为单位的内存数量,将其转换为以byte为单位
   mov cx,0x400	     ;cx和ax值一样,cx用做乘数
   mul cx 
   shl edx,16
   and eax,0x0000FFFF
   or edx,eax
   add edx, 0x100000 ;ax只是15MB,故要加1MB
   mov esi,edx	     ;先把低15MB的内存容量存入esi寄存器备份

;2 再将16MB以上的内存转换为byte为单位,寄存器bx和dx中是以64KB为单位的内存数量
   xor eax,eax
   mov ax,bx		
   mov ecx, 0x10000	;0x10000十进制为64KB
   mul ecx		;32位乘法,默认的被乘数是eax,积为64位,高32位存入edx,低32位存入eax.
   add esi,eax		;由于此方法只能测出4G以内的内存,故32位eax足够了,edx肯定为0,只加eax便可
   mov edx,esi		;edx为总内存大小
   jmp .mem_get_ok

;-----------------  int 15h ah = 0x88 获取内存大小,只能获取64M之内  ----------
.e801_failed_so_try88: 
   ;int 15后，ax存入的是以kb为单位的内存容量
   mov  ah, 0x88
   int  0x15
   jc .error_hlt
   and eax,0x0000FFFF
      
   ;16位乘法，被乘数是ax,积为32位.积的高16位在dx中，积的低16位在ax中
   mov cx, 0x400     ;0x400等于1024,将ax中的内存容量换为以byte为单位
   mul cx
   shl edx, 16	     ;把dx移到高16位
   or edx, eax	     ;把积的低16位组合到edx,为32位的积
   add edx,0x100000  ;0x88子功能只会返回1MB以上的内存,故实际内存大小要加上1MB

.mem_get_ok:
   mov [total_mem_bytes], edx	 ;将内存换为byte单位后存入total_mem_bytes处。
   
; ============= 打印字符，表示在 loader 中 ================================================
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
	
.error_hlt:		      ;出错则挂起
   hlt

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

; ============= 加载内核至 0x70000 ===================================
	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR
	mov ecx, 200		; 200 个扇区
	call rd_disk_m_32
	
;======== 分 3 步开启分页机制 ========================================	
; 第 1 步： 创建页目录表 和 页表
	call setup_page			; 创建二级页表
	
; -------------修改 GDT 里的段描述符----------------
	sgdt [gdt_ptr]
	
	mov ebx, [gdt_ptr + 2]		; 描述符表的偏移
	or dword [ebx + 0x18 + 4], 0xc0000000
	
	add dword [gdt_ptr + 2], 0xc0000000
	
	add esp, 0xc0000000

; 第 2 步: 页目录表的地址赋给 cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
; 第 3 步: 打开 cr0 的 PG 位开启分页
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	
	lgdt [gdt_ptr]			; 开启分页后重新加载 GDT
	
	mov byte [gs:320], 'V'
	mov byte [gs:321], 0x97
	
	jmp SELECTOR_CODE:enter_kernel
enter_kernel:
	call kernel_init		; 内核初始化，包括解析elf格式二进制文件
	mov esp, 0xc009f000
	jmp KERNEL_ENTRY_POINT

; ============ 内核初始化函数 ==============================================
kernel_init:
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]	  ; 偏移文件42字节处的属性是e_phentsize,表示program header大小
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]   ; 偏移文件开始部分28字节的地方是e_phoff,表示第1 个program header在文件中的偏移量
					  ; 其实该值是0x34,不过还是谨慎一点，这里来读取实际值
	add ebx, KERNEL_BIN_BASE_ADDR
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]    ; 偏移文件开始部分44字节的地方是e_phnum,表示有几个program header
.each_segment:
	cmp byte [ebx + 0], PT_NULL		  ; 若p_type等于 PT_NULL,说明此program header未使用。
	je .PTNULL

	;为函数memcpy压入参数,参数是从右往左依然压入.函数原型类似于 memcpy(dst,src,size)
	push dword [ebx + 16]		  ; program header中偏移16字节的地方是p_filesz,压入函数memcpy的第三个参数:size
	mov eax, [ebx + 4]			  ; 距程序头偏移量为4字节的位置是p_offset
	add eax, KERNEL_BIN_BASE_ADDR	  ; 加上kernel.bin被加载到的物理地址,eax为该段的物理地址
	push eax				  ; 压入函数memcpy的第二个参数:源地址
	push dword [ebx + 8]			  ; 压入函数memcpy的第一个参数:目的地址,偏移程序头8字节的位置是p_vaddr，这就是目的地址
	call mem_cpy				  ; 调用mem_cpy完成段复制
	add esp,12				  ; 清理栈中压入的三个参数
	.PTNULL:
	add ebx, edx				  ; edx为program header大小,即e_phentsize,在此ebx指向下一个program header 
	loop .each_segment
	ret
;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
;输入:栈中三个参数(dst,src,size)
;输出:无
;---------------------------------------------------------
mem_cpy:		      
	cld
	push ebp
	mov ebp, esp
	push ecx		   ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
	mov edi, [ebp + 8]	   ; dst
	mov esi, [ebp + 12]	   ; src
	mov ecx, [ebp + 16]	   ; size
	rep movsb		   ; 逐字节拷贝

	;恢复环境
	pop ecx		
	pop ebp
	ret

; =========== 创建页目录表和页表 函数 ======================================
setup_page:
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0	; PAGE_DIR_TABLE_POS = 1M ,页目录表放在内存的1M处
	inc esi
	loop .clear_page_dir					; 把页目录表 (4K) 清0

; 开始创建页目录项（PDE）page direcotry entry
.creat_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000							; 加 4K ,此时eax为第一个页表的位置及属性
	mov ebx, eax
	
	or eax, PG_US_U | PG_RW_W | PG_P
	mov [PAGE_DIR_TABLE_POS + 0x0], eax		; 页目录表的第 1 项存为第一个页表的地址
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax	; 页目录表的第 768 项存为第一个页表的地址
	
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax	; 最后一个页目录项指向自己页目录表的地址
	
; 创建页表项
	mov ecx, 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P
.creat_pte:
	mov [ebx + esi * 4], edx
	add edx, 4096
	inc esi
	loop .creat_pte
	
; 创建内核其它页表的 PTE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254
	mov esi, 769
.creat_kernel_pde:
	mov [PAGE_DIR_TABLE_POS + esi*4], eax
	add eax, 0x1000
	inc esi
	loop .creat_kernel_pde
	ret

; ================== 读取硬盘扇区 ======================================
rd_disk_m_32:	
							; eax=LBA扇区号
							; ebx=将数据写入的内存地址
							; ecx=读入的扇区数
    mov esi,eax	   ; 备份eax
    mov di,cx		   ; 备份扇区数到di
;读写硬盘:
;第1步：设置要读取的扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al            ;读取的扇区数

    mov eax,esi	   ;恢复ax

;第2步：将LBA地址存入0x1f3 ~ 0x1f6

    ;LBA地址7~0位写入端口0x1f3
    mov dx,0x1f3                       
    out dx,al                          

    ;LBA地址15~8位写入端口0x1f4
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    ;LBA地址23~16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f	   ;lba第24~27位
    or al,0xe0	   ; 设置7～4位为1110,表示lba模式
    mov dx,0x1f6
    out dx,al

;第3步：向0x1f7端口写入读命令，0x20 
    mov dx,0x1f7
    mov al,0x20                        
    out dx,al

;;;;;;; 至此,硬盘控制器便从指定的lba地址(eax)处,读出连续的cx个扇区,下面检查硬盘状态,不忙就能把这cx个扇区的数据读出来

;第4步：检测硬盘状态
.not_ready:		   ;测试0x1f7端口(status寄存器)的的BSY位
    ;同一端口,写时表示写入命令字,读时表示读入硬盘状态
    nop
    in al,dx
    and al,0x88	   ;第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘忙
    cmp al,0x08
    jnz .not_ready	   ;若未准备好,继续等。

;第5步：从0x1f0端口读数据
    mov ax, di	   ;以下从硬盘端口读数据用insw指令更快捷,不过尽可能多的演示命令使用,
			   ;在此先用这种方法,在后面内容会用到insw和outsw等

    mov dx, 256	   ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字,共需di*512/2次,所以di*256
    mul dx
    mov cx, ax	   
    mov dx, 0x1f0
.go_on_read:
    in ax,dx		
    mov [ebx], ax
    add ebx, 2
			  ; 由于在实模式下偏移地址为16位,所以用bx只会访问到0~FFFFh的偏移。
			  ; loader的栈指针为0x900,bx为指向的数据输出缓冲区,且为16位，
			  ; 超过0xffff后,bx部分会从0开始,所以当要读取的扇区数过大,待写入的地址超过bx的范围时，
			  ; 从硬盘上读出的数据会把0x0000~0xffff的覆盖，
			  ; 造成栈被破坏,所以ret返回时,返回地址被破坏了,已经不是之前正确的地址,
			  ; 故程序出会错,不知道会跑到哪里去。
			  ; 所以改为ebx代替bx指向缓冲区,这样生成的机器码前面会有0x66和0x67来反转。
			  ; 0X66用于反转默认的操作数大小! 0X67用于反转默认的寻址方式.
			  ; cpu处于16位模式时,会理所当然的认为操作数和寻址都是16位,处于32位模式时,
			  ; 也会认为要执行的指令是32位.
			  ; 当我们在其中任意模式下用了另外模式的寻址方式或操作数大小(姑且认为16位模式用16位字节操作数，
			  ; 32位模式下用32字节的操作数)时,编译器会在指令前帮我们加上0x66或0x67，
			  ; 临时改变当前cpu模式到另外的模式下.
			  ; 假设当前运行在16位模式,遇到0X66时,操作数大小变为32位.
			  ; 假设当前运行在32位模式,遇到0X66时,操作数大小变为16位.
			  ; 假设当前运行在16位模式,遇到0X67时,寻址方式变为32位寻址
			  ; 假设当前运行在32位模式,遇到0X67时,寻址方式变为16位寻址.

    loop .go_on_read
    ret
	