BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
CC = gcc
AS = nasm
LD = ld
LIB = -I kernel/
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
ASFLAGS = -f elf
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/print.o  

############   C 代码编译  #################
$(BUILD_DIR)/main.o: kernel/main.c 
	$(CC) $(CFLAGS) $< -o $@

	

############   汇编代码编译  #################
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@


	
########### 链接所有目录文件 ################
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : hd clean all

hd :
	dd if=$(BUILD_DIR)/kernel.bin of=/home/book/bochs/bochs-2.6.9/OS_Jia.img bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: build hd
