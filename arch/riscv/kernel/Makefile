BUILD=../../../build
LKOBJS = $(wildcard ../../../build/*.o)
OBJS = \
	buffer.o \
	disk.o \
	entry.o \
	head.o \
	sched.o \
	sleeplock.o \
	spinlock.o \
	strap.o \
	vm.o \
	syscall.o \
	sysproc.o \
	timer.o \
	plic.o \

ifeq ($(PLATFORM), k210)
OBJS += \
	spi.o \
	gpiohs.o \
	fpioa.o \
	utils.o \
	sdcard.o \
	dmac.o \
	sysctl.o \

else
OBJS += \
	virtio_disk.o \

endif


kernel:$(OBJS)

head.o:
	$(GCC) $(CFLAG) -c head.S -o $(BUILD)/head.o

entry.o:
	$(GCC) $(CFLAG) -c entry.S -o $(BUILD)/entry.o

strap.o:
	$(GCC) $(CFLAG) -c strap.c -o $(BUILD)/strap.o

sched.o:
	$(GCC) $(CFLAG) -c sched.c -o $(BUILD)/sched.o

vm.o:
	$(GCC) $(CFLAG) -c vm.c -o $(BUILD)/vm.o

sysctl.o:
	$(GCC) $(CFLAG) -c sysctl.c -o $(BUILD)/sysctl.o

syscall.o:
	$(GCC) $(CFLAG) -c syscall.c -o $(BUILD)/syscall.o

sysproc.o:
	$(GCC) $(CFLAG) -c sysproc.c -o $(BUILD)/sysproc.o

dmac.o:
	$(GCC) $(CFLAG) -c dmac.c -o $(BUILD)/dmac.o

buffer.o:
	$(GCC) $(CFLAG) -c buffer.c -o $(BUILD)/buffer.o

fpioa.o:
	$(GCC) $(CFLAG) -c fpioa.c -o $(BUILD)/fpioa.o

gpiohs.o:
	$(GCC) $(CFLAG) -c gpiohs.c -o $(BUILD)/gpiohs.o

sdcard.o:
	$(GCC) $(CFLAG) -c sdcard.c -o $(BUILD)/sdcard.o

sleeplock.o:
	$(GCC) $(CFLAG) -c sleeplock.c -o $(BUILD)/sleeplock.o

spi.o:
	$(GCC) $(CFLAG) -c spi.c -o $(BUILD)/spi.o

spinlock.o:
	$(GCC) $(CFLAG) -c spinlock.c -o $(BUILD)/spinlock.o

utils.o:
	$(GCC) $(CFLAG) -c utils.c -o $(BUILD)/utils.o

disk.o:
	$(GCC) $(CFLAG) -c disk.c -o $(BUILD)/disk.o

fat32.o:
	$(GCC) $(CFLAG) -c fat32.c -o $(BUILD)/fat32.o

virtio_disk.o:
	$(GCC) $(CFLAG) -c virtio_disk.c -o $(BUILD)/virtio_disk.o

timer.o:
	$(GCC) $(CFLAG) -c timer.c -o $(BUILD)/timer.o

plic.o:
	$(GCC) $(CFLAG) -c plic.c -o $(BUILD)/plic.o

vmlinux: $(LDSCRIPT) $(LKOBJS)
	$(LD) -T $(LDSCRIPT) $(LKOBJS) -o ../../../vmlinux