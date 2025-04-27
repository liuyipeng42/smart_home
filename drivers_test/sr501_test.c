#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int fd;
    struct pollfd fds;
    int ret;
    char val;

    // 打开设备文件
    fd = open("/dev/sr501", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open /dev/sr501");
        return 1;
    }

    // 配置poll监视的文件描述符
    fds.fd = fd;
    fds.events = POLLIN;  // 监视数据可读事件

    while (1) {
        // 调用poll，阻塞等待事件发生，超时设为1000ms（可调整）
        ret = poll(&fds, 1, 1000);
        if (ret < 0) {
            perror("poll error");
            break;
        } else if (ret == 0) {
            printf("poll timeout, no data.\n");
            continue;
        }

        // 检查事件是否就绪
        if (fds.revents & POLLIN) {
            // 读取传感器值
            if (read(fd, &val, 1) < 0) {
                perror("read error");
                break;
            }
            printf("Value: %d, Status: %s\n", val, val ? "有人" : "无人");
        }
    }

    close(fd);
    return 0;
}
