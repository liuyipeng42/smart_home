#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "unistd.h"

#define LEDOFF 0
#define LEDON 1

/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char* argv[]) {
    int fd, retvalue;
    unsigned char databuf[1];

    /* 打开led驱动 */
    fd = open("/dev/led", O_RDWR);
    if (fd < 0) {
        printf("file open failed!\n");
        return -1;
    }

    databuf[0] = atoi(argv[1]); /* 要执行的操作：打开或关闭 */
    retvalue = write(fd, databuf, sizeof(databuf));
    if (retvalue < 0) {
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }

    retvalue = close(fd); /* 关闭文件 */
    if (retvalue < 0) {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}
