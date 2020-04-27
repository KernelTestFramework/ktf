CFLAGS  := -fno-stack-protector -fno-exceptions -std=gnu99 -fno-builtin -O -g -Wall -I. -mno-red-zone -mno-mmx -mno-sse -mno-sse -ffreestanding
LDFLAGS := -nostdlib -lgcc

all: kernel.o kernel64.bin kernel32.bin

kernel32.bin: kernel64.bin
	objcopy -O elf32-i386 $< $@

kernel64.bin: head.o kernel.o
	gcc -o $@ $(CFLAGS) $^ $(LDFLAGS)
	ld -T link.ld -o $@ $^

head.o:
	gcc -c -o $@ $(CFLAGS) head.S

kernel.o:
	gcc -c -o $@ $(CFLAGS) kernel.c

clean:
	rm -f *.o *.bin
