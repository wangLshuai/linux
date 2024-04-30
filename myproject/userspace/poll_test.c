#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int main() {
  int ret = 0;
  char buffer0[64];
  char buffer1[64];
  struct pollfd fds[2];
  fds[0].fd = open("/dev/simple_char0", O_RDWR);
  if (fds[0].fd == -1)
    goto tail;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  fds[1].fd = open("/dev/simple_char1", O_RDWR);
  if (fds[1].fd == -1)
    goto tail;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  while (1) {
    ret = poll(fds, 2, -1);
    if (ret == -1)
      goto tail;

    if (fds[0].revents & POLLIN) {
      ret = read(fds[0].fd, buffer0, 63);
      if (ret < 0)
        goto tail;
      buffer0[ret] = '\0';
      printf("read form simple0: %s\n", buffer0);
    }

    if (fds[1].revents & POLLIN) {
      ret = read(fds[1].fd, buffer1, 63);
      if (ret < 0)
        goto tail;
      buffer1[ret] = '\0';
      printf("read form simple1:%s\n", buffer1);
    }
  }
tail:
  perror("poll test");
  return -1;
}