

#include "mem_hook.h"

void fun2()
{
	int i;
	for(i = 0; i < 6; i++){
		malloc(399);
	}
}

