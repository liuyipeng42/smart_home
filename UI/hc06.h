#ifndef HC06_H
#define HC06_H
#include <string>

#include <QThread>
#include <QObject>

class HC06 : public QThread {
    Q_OBJECT

    int fd_;
    int set_opt(int speed, int bits, char event, int stop);

public:
    HC06(const std::string& file);

protected:
    void run() override;

signals:
    void send_to_main(char* device, char* value);
};

#endif // HC06_H
