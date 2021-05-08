#include "riscv.h"

int start_kernel()
{
	print_logo();
	os_test();

	return 0;
}
