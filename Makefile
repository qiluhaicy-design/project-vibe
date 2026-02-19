CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
CFLAGS = -mcpu=arm1176jzf-s -fpic -ffreestanding -std=gnu99 -O2 -Wall -Wextra
LDFLAGS = -T kernel/linker.ld

all: kernel.img

kernel.elf: kernel/start.o kernel/main.o
	$(LD) $(LDFLAGS) -o $@ $^

kernel.img: kernel.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f kernel/*.o kernel.elf kernel.img

.PHONY: all clean