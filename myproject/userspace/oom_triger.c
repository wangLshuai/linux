#include <stdio.h>
#include <stdlib.h>
#define PAGES 1024
int main()
{
	while (1) {
		char *buf = malloc(4ul * 1024 * PAGES);
		if (buf) {
			printf("malloc success\n");
			for (int i = 0; i < PAGES; i++) {
				buf[4ul * 1024 * i] = 0x00;
			}
		} else {
			printf("malloc failed\n");
		}
	}

	return 0;
}