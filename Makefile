export
MAKE=make
CROSS_= riscv64-unknown-elf-
GCC=${CROSS_}gcc
AR=${CROSS_}ar
LD=${CROSS_}ld
NM=${CROSS_}nm
OBJCOPY=${CROSS_}objcopy
# PLATFORM=k210
PLATFORM=qemu
MODE=release
# MODE=debug

QEMU = qemu-system-riscv64
QEMUOPTS = -machine virt -kernel vmlinux -m 8M -nographic

# use multi-core 
# QEMUOPTS += -smp 2

QEMUOPTS += -bios $(RUSTSBI)

# import virtual disk image
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0 
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

ISA ?= rv64imafd
ABI ?= lp64

INCLUDE = -I ../include -I ../../../include
CF = -g -march=$(ISA) -mabi=$(ABI) -mcmodel=medany -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nostdinc -static -lgcc -Wl,--nmagic -Wl,--gcsections
CFLAG = ${CF} ${INCLUDE} -DSJF

ifeq ($(PLATFORM), k210)
RUSTSBI = ./bootloader/rustsbi-k210.bin
KSTART = 131072
LDSCRIPT = vmlinux.lds
else
RUSTSBI = ./bootloader/rustsbi-qemu.bin
CFLAGS += -DQEMU
LDSCRIPT = vmlinux-qemu.lds
endif


vmlinux:
	-mkdir build arch/riscv/boot
	${MAKE} -C ./init all
	${MAKE} -C ./lib all
	${MAKE} -C ./mm all
	${MAKE} -C ./arch/riscv all

	cp $(RUSTSBI) ./hJHOS.bin
	dd if=arch/riscv/boot/Image of=hJHOS.bin bs=$(KSTART) seek=1
	@echo "\033[;31m _     \033[0m\033[;33m    ___ \033[0m\033[;34m  _   _  \033[0m"
	@echo "\033[;31m| |    \033[0m\033[;33m   |_  |\033[0m\033[;34m | | | | \033[0m"
	@echo "\033[;31m| |__  \033[0m\033[;33m     | |\033[0m\033[;34m | |_| | \033[0m"
	@echo "\033[;31m| '_ \ \033[0m\033[;33m     | |\033[0m\033[;34m |  _  | \033[0m"
	@echo "\033[;31m| | | |\033[0m\033[;33m /\__/ /\033[0m\033[;34m | | | | \033[0m"
	@echo "\033[;31m|_| |_|\033[0m\033[;33m \____/ \033[0m\033[;34m \_| |_/ \033[0m"

clean:
	-@rm -rf build vmlinux arch/riscv/boot System.map hJHOS.bin
	@echo	"Clean done!"
	
fs:
	@if [ ! -f "fs.img" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=fs.img bs=512k count=512; \
		mkfs.vfat -F 32 fs.img; fi

qemu:
	-make vmlinux
	$(QEMU) $(QEMUOPTS)

qemu-debug:
	-make vmlinux
	$(QEMU) $(QEMUOPTS) -s -S

k210:
	-make vmlinux
	kflash -B dan -t hJHOS.bin