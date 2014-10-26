
#include <stdio.h>
#include "mem_hook.h"

void *ptr1[1024];

static int fun1()
{
	int i;
	for(i = 0; i < 1024; i++){
		ptr1[i] = (void*)malloc(455);
	}
}

int main(int argc, char *argv[])
{
	int i;
	memory_hook_init();
	
	for(i = 0; i < 10; i++){
		malloc(133);
	}
	fun1();
	for(i = 0; i < 1020; i++){
		free(ptr1[i]);
	}
	fun2();
	char *str = strdup("hello");
    char *str1 = strdup("1111111111");
    free(str);
    void *ptr = calloc(1002, 2);
    void *ptr2 = calloc(1002, 2);
    void *ptr3 = realloc(ptr2, 3004);
    printf("tr %x\n", ptr3+1);
    free(ptr3);
    ptr3 = realloc(NULL,100);
    ptr3 = realloc(ptr3,400);
    free(ptr);
    dump_memory(stderr);
	return 0;
}

