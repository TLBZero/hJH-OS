#include "riscv.h"

static void logoChar(){
	kprintf(".__            .___________.   .__        __.\n\
|  |           |___.   .___|   |  |      |  |\n\
|  |               |   |       |  |      |  |\n\
|  `.-----.        |   |       |  `------‘  |\n\
|   .___   \\       |   |       |    _____   |\n\
|  /    |  |       |   |       |   /     \\  |\n\
|  |    |  |    ._/   /        |  |      |  |\n\
|__|    |__|   /_____/         |__|      |__|\n");
}

int start_kernel()
{
	logoChar();
	os_test();

	return 0;
}
