#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
int ret = 0;
ret = syscall(548);
printf("ret:%d\n",ret);

  return 0;
}
