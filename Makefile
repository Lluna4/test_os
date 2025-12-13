NAME = internal-disk/EFI/BOOT/BOOTX64.EFI

FLAGS = -mno-red-zone -ffreestanding -nostdlib -Wl,--subsystem,10 

SRC = main.c

all: ${SRC} kernel.bin
	x86_64-w64-mingw32-gcc ${SRC} ${FLAGS} -e efi_main -o ${NAME}

fclean: clean
	rm -rf ${NAME}

re: fclean all

kernel.bin: kernel/main.c
	clang -c ${FLAGS} -fPIE -o kernel.o $<
	ld -nostdlib -e kernel_main --oformat binary -o $@ kernel.o
	cp $@ internal-disk/EFI/kernel/kernel.bin

.PHONY: all clean fclean re
