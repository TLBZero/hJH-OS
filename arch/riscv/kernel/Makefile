OBJS=$(wildcard ../../../build/*.o)

kernel:head.o entry.o strap.o sched.o vm.o

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

vmlinux:vmlinux.lds $(OBJS)
	$(LD) -T vmlinux.lds $(OBJS) -o ../../../vmlinux