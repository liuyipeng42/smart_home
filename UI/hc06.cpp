#include "hc06.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <QDebug>

HC06::HC06(const std::string& file) {
    fd_ = open(file.c_str(), O_RDWR | O_NOCTTY);
    if (-1 == fd_) {
        qDebug() << "Failed to open: " << file.c_str();
        return;
    }

    if (fcntl(fd_, F_SETFL, 0) < 0) {
        qDebug() << "fcntl failed!";
        return;
    }

    int ret = set_opt(9600, 8, 'N', 1);
    if (ret) {
        qDebug() << "set port err!";
    }
}

int HC06::set_opt(int speed, int bits, char event, int stop) {
    struct termios newtio, oldtio;

    if (tcgetattr(fd_, &oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
    newtio.c_oflag &= ~OPOST;                          /*Output*/

    switch (bits) {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }

    switch (event) {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E':
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':
            newtio.c_cflag &= ~PARENB;
            break;
    }

    switch (speed) {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }

    if (stop == 1)
        newtio.c_cflag &= ~CSTOPB;
    else if (stop == 2)
        newtio.c_cflag |= CSTOPB;

    newtio.c_cc[VMIN] = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
    newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间:
                             * 比如VMIN设为10表示至少读到10个数据才返回,
                             * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
                             * 假设VTIME=1，表示:
                             *    10秒内一个数据都没有的话就返回
                             *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
                             */

    tcflush(fd_, TCIFLUSH);

    if ((tcsetattr(fd_, TCSANOW, &newtio)) != 0) {
        qDebug() << "set error!";
        return -1;
    }
    // printf("set done!\n");
    return 0;
}

void HC06::run() {
    int cnt = 0;
    char buf[20] = {0};
    while (1) {
        read(fd_, &buf[cnt++], 1);

        char* start_brace = strchr(buf, '{');
        char* comma = strchr(buf, ',');
        char* end_brace = strchr(buf, '}');

        if (start_brace && comma && end_brace && start_brace < comma && comma < end_brace) {
            qDebug() << buf;

            char device[20] = {0};
            char value[20] = {0};

            strncpy(device, start_brace + 1, comma - start_brace - 1);
            strncpy(value, comma + 1, end_brace - comma - 1);

            emit send_to_main(device, value);

            cnt = 0;
            memset(buf, 0, sizeof(buf));
        }
        if (cnt == 20) {
            cnt = 0;
            memset(buf, 0, sizeof(buf));
            printf("buff full!!!!\n");
        }
    }
}
