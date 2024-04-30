#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
  char buf[64] = {0};
  int fd = open("/dev/simple_char0", O_RDWR | O_NONBLOCK);
  int ret = read(fd, buf, sizeof(buf));
  if (ret < 0)
    printf("error:%s\n", strerror(ret));
  if (ret > 0)
    printf("read %d byte:%s\n", ret, buf);
  char *s = "abcdefghijklmnopqrstuvwxyz0123456789";
  ret = write(fd, s, strlen(s));
  if (ret < 0)
    printf("error:%s\n", strerror(ret));
  else {
    printf("write %d byte\n", ret);
  }
  ret = write(fd, s, strlen(s));
  if (ret < 0)
    printf("error:%s\n", strerror(ret));
  else {
    printf("write %d byte\n", ret);
  }
  ret = write(fd, s, strlen(s));
  if (ret < 0)
    printf("error:%s\n", strerror(ret));
  else {
    printf("write %d byte\n", ret);
  }

  return 0;
}