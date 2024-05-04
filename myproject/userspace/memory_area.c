#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int g_a;
int main()
{
	int a = 0, ret;
	printf("main addr                %016lx\n", (unsigned long)main);
	printf("global variable g_a addr:%016lx\n", (unsigned long)&g_a);
	printf("local variabel a addr    %016lx\n", (unsigned long)&a);
	printf("printf addr:             %016lx\n", (unsigned long)printf);
	void *p = malloc(10);
	printf("heap pointer addr:       %016lx\n", (unsigned long)p);
	ret = syscall(548);
	printf("ret:%d\n", ret);
	return 0;
}