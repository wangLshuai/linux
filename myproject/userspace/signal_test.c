#define _GNU_SOURCE
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>


static int fd;

void signal_handle_fun(int sign, siginfo_t *siginfo, void *act) {
  int ret;
  char buf[64];

  if (sign == SIGIO) {
    if (siginfo->si_band & POLLIN) {
      printf("FIFO is not empty\n");
      if ((ret = read(fd, buf, sizeof(buf))) != -1) {
        buf[ret] = '\0';
        puts(buf);
      }
    }
    if (siginfo-> si_band & POLLOUT)
      printf("FIFO is not full\n");
  }
}

int main() {
  int ret;
  int flag;
  struct sigaction act, oldact;

  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = signal_handle_fun;
  if (sigaction(SIGIO, &act, &oldact) == -1)
    goto tail;

  fd = open("/dev/simple_char0", O_RDWR);
  if (fd < 0)
    goto tail;

  if (fcntl(fd, F_SETOWN, getpid()) == -1)
    goto tail;

  if (fcntl(fd, F_SETSIG, SIGIO) == -1)
    goto tail;

  if ((flag = fcntl(fd, F_GETFL)) == -1)
    goto tail;

  if (fcntl(fd, F_SETFL, flag | FASYNC) == -1)
    goto tail;

  while (1)
    sleep(1);

  return 0;
tail:
  return -1;
}