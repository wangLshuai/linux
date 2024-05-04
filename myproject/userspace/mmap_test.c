#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define MMAP_DEV_NAME "/dev/mmap_dev"
#define MMAP_DEV_CMD_GET_BUFSIZE 0

int main()
{
	size_t buf_size = 0;
	char *read_buf;
	int fd = open(MMAP_DEV_NAME, O_RDWR);
	if (fd < 0) {
		printf("open device %s failed:%s\n", MMAP_DEV_NAME,
		       strerror(errno));
		return fd;
	}
	if (ioctl(fd, MMAP_DEV_CMD_GET_BUFSIZE, &buf_size)) {
		printf("MMAP_DEV_CMD_GET_BUFSIZE failed:%s\n", strerror(errno));
		return -1;
	}
	printf("bufsize:%lu\n", buf_size);

	char *map_buf =
		mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_buf == MAP_FAILED) {
		printf("mmap failed:%s\n", strerror(errno));
		return -1;
	}
	for (int i = 0; i < buf_size; i++) {
		map_buf[i] = 0x55;
	}
	munmap(map_buf, buf_size);

	read_buf = malloc(buf_size);
	if (!read_buf) {
		printf("malloc %lu bytes faialed\n", buf_size);
		return -1;
	}
	ssize_t ret = read(fd, read_buf, buf_size);
	printf("read %lu byte\n", ret);
	for (ssize_t i = 0; i < ret; i++) {
		if (read_buf[i] != 0x55) {
			printf("%lu'st byte is %x\n", i, read_buf[i] & 0x00ff);
		}
	}
	close(fd);
	return 0;
}