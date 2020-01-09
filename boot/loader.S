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

gdt_ptr							; GDT 寄存器中要存的
	dw GDT_LIMIT
	dd GDT_BASE

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
	
	jmp $

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
.creat_kernel_pde
	mov [PAGE_DIR_TABLE_POS + esi*4], eax
	add eax, 0x1000
	inc esi
	loop .creat_kernel_pde
	ret
	
	
	