#include "test.h"

int os_test()
{
	const char *msg = "ZJU OS Con Trying 			:) \n";

	puts(msg);
	task_init();
	while(1);
	return 0;
}
