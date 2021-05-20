OBJS=$(wildcard ../../../build/*.o)

kernel:head.o entry.o strap.o sched.o vm.o sysctl.o buffer.o dmac.o fpioa.o gpiohs.o sdcard.o sleeplock.o spi.o spinlock.o utils.o syscall.o sysproc.o

head.o:
	$(GCC) $(CFLAG) -c head.S -o ../../../build/head.o

entry.o:
	$(GCC) $(CFLAG) -c entry.S -o ../../../build/entry.o

strap.o:
	$(GCC) $(CFLAG) -c strap.c -o ../../../build/strap.o

sched.o:
	$(GCC) $(CFLAG) -c sched.c -o ../../../build/sched.o

vm.o:
	$(GCC) $(CFLAG) -c vm.c -o ../../../build/vm.o

sysctl.o:
	$(GCC) $(CFLAG) -c sysctl.c -o ../../../build/sysctl.o

syscall.o:
	$(GCC) $(CFLAG) -c syscall.c -o ../../../build/syscall.o

sysproc.o:
	$(GCC) $(CFLAG) -c sysproc.c -o ../../../build/sysproc.o
dmac.o:
	$(GCC) $(CFLAG) -c dmac.c -o ../../../build/dmac.o

buffer.o:
	$(GCC) $(CFLAG) -c buffer.c -o ../../../build/buffer.o

fpioa.o:
	$(GCC) $(CFLAG) -c fpioa.c -o ../../../build/fpioa.o

gpiohs.o:
	$(GCC) $(CFLAG) -c gpiohs.c -o ../../../build/gpiohs.o

sdcard.o:
	$(GCC) $(CFLAG) -c sdcard.c -o ../../../build/sdcard.o

sleeplock.o:
	$(GCC) $(CFLAG) -c sleeplock.c -o ../../../build/sleeplock.o

spi.o:
	$(GCC) $(CFLAG) -c spi.c -o ../../../build/spi.o

spinlock.o:
	$(GCC) $(CFLAG) -c spinlock.c -o ../../../build/spinlock.o

utils.o:
	$(GCC) $(CFLAG) -c utils.c -o ../../../build/utils.o

vmlinux:vmlinux.lds $(OBJS)
	$(LD) -T vmlinux.lds $(OBJS) -o ../../../vmlinux