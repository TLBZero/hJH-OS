#include "test.h"
#include "put.h"

int os_test()
{
	const char *msg = "ZJU OS Con Trying 			:) \n";

	printf(msg);
	task_init();
	while(1);
	return 0;
}
