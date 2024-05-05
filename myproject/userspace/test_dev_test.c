#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TEST_IOC_MEM _IO('C', 0x01)
#define TEST_IOC_TASK _IO('C', 0x02)

int main(int argc, char *argv[])
{
	int fd = open("/dev/test_dev", O_RDWR);
	if (fd < 0) {
		printf("open failed:%s\n", strerror(errno));
		return -1;
	}

	if (argc == 2) {
		long i = strtol(argv[1], NULL, 10);
		printf("test ioctl num :%ld\n", i);
		switch (i) {
		case 1:
			if (ioctl(fd, TEST_IOC_MEM, NULL) == -1) {
				printf("ioctl failed:%s\n", strerror(errno));
			}
			break;
		case 2:
			if (ioctl(fd, TEST_IOC_TASK, NULL) == -1) {
				printf("ioctl failed:%s\n", strerror(errno));
			}
			break;
		default:
			printf("invalid argument\n./userspace/test_dev_test 1\n");
			break;
		}
	}

	close(fd);
	return 0;
}