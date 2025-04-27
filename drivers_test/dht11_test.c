#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int fd;
    int res;
    unsigned char buf[5];  // 改为5个字节，类型改为 unsigned char 以避免负数显示问题

    fd = open("/dev/dht11", O_RDWR);

    if (fd < 0) {
        printf("dht11 open failed\n");
        return 0;
    }

    /* DHT11上电后，要等待1秒以越过不稳定状态，在此期间不能发送任何指令。 */
    sleep(1);

    while (1) {
        /* DHT11属于低速传感器，两次通信请求之间的间隔时间不能太短，一般不低于1秒。 */
        res = read(fd, buf, 5);
        if (res == 0) {
            printf("read success\n");
            printf("DHT11 Temperature: %d.%d°C, Humidity: %d.%d%%\n", buf[2], buf[3], buf[0], buf[1]);
        } else {
            printf("read failed\n");
        }
        sleep(3);
    }

    close(fd);
    return 0;
}
