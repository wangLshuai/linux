#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>

int main(int argc,char *argv[]) {
    // 打开键盘设备，这里假设/dev/input/event0是键盘设备
    char buf[64];
    snprintf(buf,sizeof(buf),"/dev/input/event%s",argv[1]);
    printf("%s\n",buf);
    int fd = open(buf, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open keyboard device");
        return 1;
    }

    struct input_event ev;
    printf("sizeof(input_event):%ld\n",sizeof(ev));
    while (1) {
        // 设置poll结构体，监听键盘设备是否有可读事件
        struct pollfd fds[1] = {{fd, POLLIN, 0}};

        // 阻塞等待键盘事件发生
        int poll_result = poll(fds, 1, -1); // -1表示无限等待
        if (poll_result == -1) {
            perror("poll failed");
            break;
        }

        // 读取键盘事件
        while (read(fd, &ev, sizeof(struct input_event)) == sizeof(struct input_event)) {
            // 检查事件类型
//            if (ev.type == EV_KEY || ev.type == EV_REL) {
                // 输出按键信息
                printf("Event: time %ld.%ld, type %d, code %d, value %d\n",
                       ev.time.tv_sec, ev.time.tv_usec,
                       ev.type, ev.code, ev.value);
 //           }
        }
    }

    close(fd);
    return 0;
}
