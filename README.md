#### 1. 让 MBR先飞一会儿

mbr.S 加载到 0x7c00 处执行，清屏，移动光标，打印字符串 蓝底白字的“OS_Jia”。

#### 2. 改进MBR ,直接操作显卡

mbr.S 加载到 0x7c00 处执行，清屏，直接往显存放入要显示的字符及属性，打印输出 蓝底红字的“OS_Jia”。

#### 3. 改进MBR, 读硬盘

mbr.S 添加读硬盘的函数，把第二扇区里的loader读到0x9000处，最后跳转至0x9000处执行。

添加 loader.S 生成loader.bin 下载到硬盘的第二扇区。

#### 4. 改进loader.S 进入保护模式

在 loader.S 中构建 GDT ，然后分3步进入保护模式：

- 打开 A20 地址线
- 加载 GDTR 寄存器
- cr0 寄存器的第 0 位置 1。

#### 5. 获取物理内存容量

在进入保护模式之前，利用 BIOS 中断来获取物理内存容量，并放在 loader的第 0x203地址处（loader要加载到 0x900 处，所以最终内存容量的数值放在 0xb03 内存地址处）。（通过 xp 0xb03 查看）

#### 6. 启用分页机制

创建页目录表和页表，页目录表地址赋给cr3, 打开cr0的pg 位，开启分页机制。（要修改GDT中的内容和栈指针）

#### 7. 加载内核

编写 main.c ，用gcc 编译、ld 链接 为 elf 格式的文件（kernel.bin）。

把内核kernel.bin从9扇区读到物理地址0x7000处, 然后解析elf格式文件，加载到0xc0001500，栈顶改为0xc009fc00。

最终跳转到内核（0xc0001500）执行。

#### 8. 实现打印字符

添加print.S 和 std_int.h ，在main.c 中调用 put_char 实现打印单个字符。

#### 9. 实现打印字符串

利用put_char，封装一个put_str函数实现打印字符串。

#### 10. 实现打印整数

利用put_char，继续封装函数put_int 实现打印16进制整数。

#### 11. 编写中断处理程序

构建IDT（中断描述符表）、初始化8259A、加载 idtr 寄存器，打开中断。

#### 12. 调整中断处理程序

从汇编跳到c程序执行中断处理。

#### 13. 实现 ASSERT 断言

添加文件debug.h 和 debug.c 实现 ASSERT。

#### 14. 实现字符串操作函数

添加文件 string.h 和 string.c 实现字符串操作的一系列函数。

#### 15.实现位图

添加文件 bitmap.h 和文件 bitmap.c 实现位图结构，位图的相关操作。

#### 16. 内存初始化

初始化 1. 内核物理内存池 2. 用户物理内存池 3. 内核虚拟内存池。

#### 17. 内存管理

申请虚拟内存、申请物理内存，添加页目录表和页表。

#### 18. 实现线程

添加文件 thread.h 和 thread.c ，实现中断栈、线程栈、PCB结构。并在init_thread 中初始化PCB结构、在thread_creat中初始化线程栈，thread_start中创建线程PCB。最终实现线程。

#### 19. 实现双向链表

添加 list.c 和 list.h 文件，实现双向链表，为接下来的线程调度做准备。

#### 20. 线程调度

增加timer.c 和timer.h 文件 中断注册函数，thread.c中增加，线程调度，主线程注册等函数。

#### 21. 同步机制 -- 锁

用信号量实现锁机制，添加文件sync.c 和sync.h 实现锁的操作，文件 console.c 实现终端输出（利用了锁）。

#### 22. 从键盘获取输入

识别键盘中断，中断处理函数就是向终端输出 ‘K’，关闭时钟中断。

#### 23. 键盘驱动

修改键盘中断处理函数，实现键盘驱动程序。

#### 24. 实现 tss

定义 tss结构体，并初始化放在 GDT中。重新加载 GDTR，加载 TR。

#### 25. 实现用户进程

- 创建进程

  在内核空间申请 1 页做进程PCB，调用init_thread初始化进程PCB，再在内核空间申请 n 页做用户虚拟地址位图，调用thread_creat初始化PCB中的线程栈，最后加入到就绪队列和全部队列中。

- 执行进程

  中断发生，执行到schedule，在schedule里激活页表，调用swith_to，最终执行kernel_thread，kernel_thread又调用start_process， start_process里设置中断栈，最后跳转到intr_exit执行。模拟中断返回进入到特权级更低的用户进程。