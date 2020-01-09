AS = nasm

mbr.bin: mbr.S
	$(AS) -o $@ $<


.PHONY : hd build clean all

hd :
	dd if=mbr.bin of=/home/book/bochs/bochs-2.6.9/hd60M.img bs=512 count=1 conv=notrunc

clean:
	rm -f *.bin

build: mbr.bin

all: build hd