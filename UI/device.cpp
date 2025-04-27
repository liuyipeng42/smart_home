#include "device.h"
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>

Device::Device() : fd_(-1), state_(0) {}

Device::Device(const std::string& file): fd_(-1), state_(0) {
    fd_ = open(file.c_str(), O_RDWR);
    qDebug() << file.c_str() << " " << fd_;
    if (fd_ < 0) {
        qDebug() << "Failed to open: " << file.c_str();
    }
}

Device::~Device() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

unsigned char Device::state() {
    return state_;
}

int Device::fd() {
    return fd_;
}

void Device::set_state(int state) {
    state_ = state;
}

void Device::write(unsigned char value) {
    unsigned char databuf[1] = {value};
    int ret = ::write(fd_, databuf, sizeof(databuf));
    if (ret < 0) {
        return;
    }
}
