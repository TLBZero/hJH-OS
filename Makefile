export
MAKE=make
CROSS_= riscv64-unknown-elf-
GCC=${CROSS_}gcc
AR=${CROSS_}ar
LD=${CROSS_}ld
NM=${CROSS_}nm
OBJCOPY=${CROSS_}objcopy

ISA ?= rv64imafd
ABI ?= lp64

INCLUDE = -I ../include -I ../../../include
CF = -g -march=$(ISA) -mabi=$(ABI) -mcmodel=medany -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nostdinc -static -lgcc -Wl,--nmagic -Wl,--gcsections
CFLAG = ${CF} ${INCLUDE} -DSJF

vmlinux:
	make clean
	-mkdir build arch/riscv/boot
	${MAKE} -C ./init all
	${MAKE} -C ./lib all
	${MAKE} -C ./mm all
	${MAKE} -C ./arch/riscv all

	cp bootloader/rustsbi-k210.bin ./hJHOS.bin
	dd if=arch/riscv/boot/Image of=hJHOS.bin bs=131072 seek=1
	@echo "\033[;31m _     \033[0m\033[;33m    ___ \033[0m\033[;34m  _   _  \033[0m"
	@echo "\033[;31m| |    \033[0m\033[;33m   |_  |\033[0m\033[;34m | | | | \033[0m"
	@echo "\033[;31m| |__  \033[0m\033[;33m     | |\033[0m\033[;34m | |_| | \033[0m"
	@echo "\033[;31m| '_ \ \033[0m\033[;33m     | |\033[0m\033[;34m |  _  | \033[0m"
	@echo "\033[;31m| | | |\033[0m\033[;33m /\__/ /\033[0m\033[;34m | | | | \033[0m"
	@echo "\033[;31m|_| |_|\033[0m\033[;33m \____/ \033[0m\033[;34m \_| |_/ \033[0m"
clean:
	-@rm -rf build vmlinux arch/riscv/boot System.map hJHOS.bin
	@echo	"Clean done!"