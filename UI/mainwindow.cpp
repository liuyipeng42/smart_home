#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include <QTimer>
#include <signal.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    this->setGeometry(900, 500, 800, 480);

    QFont title_font = QFont();
    title_font.setPointSize(25);
    title_font.setBold(true);
    title_ = new QLabel(this);
    title_->setText("Smart Home");
    title_->setFont(title_font);
    title_->setGeometry(10, 10, 780, 50);
    title_->setAlignment(Qt::AlignCenter);
    title_->setStyleSheet("background-color: rgb(173, 216, 230);");

    QFont btn_font = QFont();
    btn_font.setPointSize(20);
    led_btn_ = new QPushButton("灯光", this);
    led_btn_->setGeometry(88, 75, 150, 70);
    led_btn_->setFont(btn_font);
    beep_btn_ = new QPushButton("门铃", this);
    beep_btn_->setGeometry(325, 75, 150, 70);
    beep_btn_->setFont(btn_font);
    door_btn_ = new QPushButton("门锁", this);
    door_btn_->setGeometry(563, 75, 150, 70);
    door_btn_->setFont(btn_font);

    led_ = new Device("/dev/led");
    connect(led_btn_, SIGNAL(clicked()), this, SLOT(LEDBtnClicked()));
    beep_ = new Device("/dev/beep");
    connect(beep_btn_, SIGNAL(clicked()), this, SLOT(BeepBtnClicked()));
    door_ = new Device("/dev/relay");
    connect(door_btn_, SIGNAL(clicked()), this, SLOT(DoorBtnClicked()));
    curtain_ = new Device("/dev/sg90");
    connect(door_btn_, SIGNAL(clicked()), this, SLOT(DoorBtnClicked()));

    QFont curtain_font = QFont();
    curtain_font.setPointSize(21);
    curtain_lable_ = new QLabel(this);
    curtain_lable_->setText("窗帘");
    curtain_lable_->setFont(curtain_font);
    curtain_lable_->setGeometry(110, 190, 100, 50);

    curtain_degree_ = new QProgressBar(this);
    curtain_degree_->setGeometry(190, 190, 500, 30);
    curtain_degree_->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #AAAAAA;"
        "    text-align: center;"
        "    color: rgb(255, 0, 0);"
        "    font: bold 20px;"
        "    border-radius: 10px;"
        "    padding: 1px;"
        "}"
        "QProgressBar::chunk {"
        "    border-radius: 5px;"
        "    background-color: skyblue;"
        "    width: 10px;"
        "    margin: 1px;"
        "}"
    );
    curtain_degree_->setRange(0, 100);
    curtain_degree_->setValue(0);
    curtain_degree_->setFormat("%p%");

    curtain_slider_ = new QSlider(Qt::Horizontal, this);
    curtain_slider_->setGeometry(190, 225, 500, 30);
    curtain_slider_->setRange(0, 100);
    curtain_slider_->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 15px;
            background: #ccc;
            border-radius: 5px;
        }
        QSlider::handle:horizontal {
            width: 30px;
            height: 35px;
            background: #0078D7;
            border: 1px solid #5c5c5c;
            margin: -5px 0; /* 为了使handle垂直居中 */
            border-radius: 10px;
        }
    )");

    connect(curtain_slider_, SIGNAL(sliderReleased()), this, SLOT(CurtainSliderChanged()));

    QFont sensor_font = QFont();
    sensor_font.setPointSize(20);

    IR_ = new QLabel(this);
    IR_->setText("红外");
    IR_->setFont(sensor_font);
    IR_->setGeometry(88, 280, 100, 50);
    IR_num_ = new QLabel(this);
    IR_num_->setNum(0);
    IR_num_->setFont(sensor_font);
    IR_num_->setGeometry(168, 280, 100, 50);

    ALS_ = new QLabel(this);
    ALS_->setText("光照");
    ALS_->setFont(sensor_font);
    ALS_->setGeometry(325, 280, 100, 50);
    ALS_num_ = new QLabel(this);
    ALS_num_->setNum(0);
    ALS_num_->setFont(sensor_font);
    ALS_num_->setGeometry(405, 280, 100, 50);

    PS_ = new QLabel(this);
    PS_->setText("距离");
    PS_->setFont(sensor_font);
    PS_->setGeometry(563, 280, 100, 50);
    PS_num_ = new QLabel(this);
    PS_num_->setNum(0);
    PS_num_->setFont(sensor_font);
    PS_num_->setGeometry(643, 280, 100, 50);

    ap3216c_ = new Device("/dev/ap3216c");

    temperature_ = new QLabel(this);
    temperature_->setText("温度");
    temperature_->setFont(sensor_font);
    temperature_->setGeometry(80, 350, 100, 50);
    temperature_num_ = new QLabel(this);
    temperature_num_->setText("0");
    temperature_num_->setFont(sensor_font);
    temperature_num_->setGeometry(160, 350, 100, 50);

    humidity_ = new QLabel(this);
    humidity_->setText("湿度");
    humidity_->setFont(sensor_font);
    humidity_->setGeometry(240, 350, 100, 50);
    humidity_num_ = new QLabel(this);
    humidity_num_->setText("0");
    humidity_num_->setFont(sensor_font);
    humidity_num_->setGeometry(320, 350, 100, 50);

    dht11_ = new Device("/dev/dht11");

    human_ = new QLabel(this);
    human_->setText("有人");
    human_->setFont(sensor_font);
    human_->setGeometry(400, 350, 100, 50);
    human_num_ = new QLabel(this);
    human_num_->setNum(0);
    human_num_->setFont(sensor_font);
    human_num_->setGeometry(480, 350, 100, 50);

    sr501_ = new Device("/dev/sr501");

    socketNotifier_ = new QSocketNotifier(sr501_->fd(), QSocketNotifier::Read, this);
    connect(socketNotifier_, &QSocketNotifier::activated, this, &MainWindow::sr501_handler);

    air_ = new QLabel(this);
    air_->setText("空气");
    air_->setFont(sensor_font);
    air_->setGeometry(560, 350, 100, 50);
    air_num_ = new QLabel(this);
    air_num_->setNum(0);
    air_num_->setFont(sensor_font);
    air_num_->setGeometry(640, 350, 100, 50);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(Update()));
    timer->start(300); // 每秒触发一次

    hc06_ = new HC06("/dev/ttymxc2");
    connect(hc06_, SIGNAL(send_to_main(char*, char* )), this, SLOT(reveive_from_hc06(char*, char* )));
    hc06_->start();
}

void MainWindow::LEDBtnClicked() {
    led_->set_state(!led_->state());
    led_->write(led_->state());
}

void MainWindow::BeepBtnClicked() {
    beep_->set_state(!beep_->state());
    beep_->write(beep_->state());
}

void MainWindow::DoorBtnClicked() {
    door_->set_state(!door_->state());
    door_->write(door_->state());
}

void MainWindow::CurtainSliderChanged() {
    int value = curtain_slider_->value() ;
    curtain_degree_->setValue(value);
    curtain_->write((unsigned char)(180 * (value / 100.0)));
}

void MainWindow::Update() {
    // 0.3s读取ap3216c
    unsigned short ir, als, ps;
    int ret = 0;
    unsigned short ap3216c_buf[3];
    ret = read(ap3216c_->fd(), ap3216c_buf, sizeof(ap3216c_buf));
    if (ret == 0) {
        ir = ap3216c_buf[0];
        als = ap3216c_buf[1];
        ps = ap3216c_buf[2];
        IR_num_->setNum(ir);
        ALS_num_->setNum(als);
        PS_num_->setNum(ps);
    }

    // 3秒读取一次dht11
    if (timer_cnt_ == 0) {
        unsigned char dht11_buf[5];
        ret = read(dht11_->fd(), dht11_buf, sizeof(dht11_buf));
        if (ret == 0) {
            temperature_num_->setText(QString("%1.%2°C").arg(dht11_buf[2]).arg(dht11_buf[3]));
            humidity_num_->setText(QString("%1.%2%").arg(dht11_buf[0]).arg(dht11_buf[1]));
        } else {
            qDebug() << "read failed";
        }
    }

    // 1.5秒读一次adc
    if (timer_cnt_ == 0 || timer_cnt_ == 5) {
        FILE * raw_fd = fopen("/sys/bus/iio/devices/iio:device0/in_voltage1_raw", "r");
        char data[20];
        fscanf(raw_fd, "%s", data);
        air_num_->setNum(atoi(data));
        fclose(raw_fd);
    }

    timer_cnt_ ++;
    if (timer_cnt_ == 10)
        timer_cnt_ = 0;
}

void MainWindow::sr501_handler() {
    char val;
    read(sr501_->fd(), &val, 1);
    human_num_->setNum(val);
}

void MainWindow::reveive_from_hc06(char* device,  char* value) {
    qDebug() << "device:" << device << " value:" << value;

    if (strcmp(device, "led") == 0) {
        led_->write(static_cast<unsigned char>(atoi(value)));
    }
    if (strcmp(device, "beep") == 0) {
        beep_->write(static_cast<unsigned char>(atoi(value)));
    }
    if (strcmp(device, "door") == 0) {
        door_->write(static_cast<unsigned char>(atoi(value)));
    }
    if (strcmp(device, "curtain") == 0) {
        int int_value = atoi(value);
        curtain_->write(static_cast<unsigned char>(180 *  (int_value / 100.0)));
        curtain_degree_->setValue(int_value);
        curtain_slider_->setValue(int_value);
    }
}

MainWindow::~MainWindow() {
    delete ui_;
    led_->~Device();
    beep_->~Device();
    door_->~Device();
    curtain_->~Device();
    ap3216c_->~Device();
    dht11_->~Device();
    sr501_->~Device();
}

