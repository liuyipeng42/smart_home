#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int fd;
    int res;
    unsigned char buf[1];

    fd = open("/dev/sg90", O_WRONLY);

    if (fd < 0) {
        printf("sg90 open failed\n");
        return 0;
    }

    buf[0] = atoi(argv[1]);
    write(fd, buf, 1);

    close(fd);
    return 0;
}
