BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
CC = gcc
AS = nasm
LD = ld
LIB = -I kernel/ -I lib/ -I lib/kernel/ -I thread/ -I device/
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes
ASFLAGS = -f elf
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/init.o $(BUILD_DIR)/print.o \
		$(BUILD_DIR)/interrupt.o $(BUILD_DIR)/io.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o \
		$(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/console.o $(BUILD_DIR)/sync.o \
		$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o

############   C 代码编译  #################
$(BUILD_DIR)/main.o: kernel/main.c kernel/interrupt.c kernel/init.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h 
	$(CC) $(CFLAGS) -fno-stack-protector $< -o $@
	
$(BUILD_DIR)/io.o: lib/kernel/io.c
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/debug.o : kernel/debug.c 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c	kernel/global.h lib/string.h lib/std_int.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h kernel/global.h lib/string.h lib/std_int.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/kernel/bitmap.c lib/kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h kernel/memory.c kernel/global.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/list.o: lib/kernel/list.c lib/kernel/list.h kernel/global.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/timer.o: device/timer.c device/timer.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/console.o: device/console.c device/console.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/sync.o: thread/sync.c thread/sync.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@
	
$(BUILD_DIR)/tss.o: userprog/tss.c userprog/tss.h
	$(CC) $(CFLAGS) -fno-stack-protector $< -o $@
	
$(BUILD_DIR)/process.o: userprog/process.c userprog/process.h
	$(CC) $(CFLAGS) $< -o $@

############   汇编代码编译  #################
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o: thread/switch.S
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
