#ifndef DEVICE_H
#define DEVICE_H

#include <string>

class Device {
    int fd_;
    unsigned char state_ = 0;

   public:
    Device();
    Device(const std::string& file);
    ~Device();

    unsigned char state();
    int fd();
    void set_state(int state);
    void write(unsigned char value);
};

#endif  // DEVICE_H
