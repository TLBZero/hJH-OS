all:
	$(MAKE) -C ./kernel kernel
	$(MAKE) -C ./kernel vmlinux
	cd ../.. ;$(OBJCOPY) -O binary vmlinux arch/riscv/boot/Image --strip-all;$(NM) vmlinux > System.map