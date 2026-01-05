NAME = internal-disk/EFI/BOOT/BOOTX64.EFI

FLAGS = -mno-red-zone -ffreestanding -nostdlib -Wl,--subsystem,10 

SRC = main.c

all: ${SRC} kernel.bin
	x86_64-w64-mingw32-gcc ${SRC} ${FLAGS} -e efi_main -o ${NAME}
	qemu-system-x86_64 --bios /usr/share/edk2/ovmf/OVMF_CODE.fd -m 250M -netdev user,id=net0 -device rtl8139,netdev=net0,mac=12:34:56:78:9a:bc -drive format=raw,file=fat:rw:internal-disk
	
fclean: clean
	rm -rf ${NAME}

re: fclean all

kernel.bin: kernel/main.c
	clang -c ${FLAGS} -mgeneral-regs-only -fPIE -o kernel.o $<
	clang -c ${FLAGS} -mgeneral-regs-only -fPIE -o utils.o kernel/utils.c
	ld -T kernel/kernel.ld -nostdlib -e kernel_main --oformat binary -o $@ kernel.o utils.o
	cp $@ internal-disk/EFI/kernel/kernel.bin

.PHONY: all clean fclean re
