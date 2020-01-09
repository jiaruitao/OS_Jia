AS = nasm

mbr.bin: mbr.S
	$(AS) -o $@ $<

loader.bin: loader.S
	$(AS) -o $@ $<

.PHONY : hd build clean all hd_loader

hd:
	dd if=mbr.bin of=/home/book/bochs/bochs-2.6.9/OS_Jia.img bs=512 count=1 conv=notrunc

hd_loader:
	dd if=loader.bin of=/home/book/bochs/bochs-2.6.9/OS_Jia.img bs=512 count=1 seek=2 conv=notrunc
clean:
	rm -f *.bin

build: mbr.bin loader.bin

all: build hd hd_loader