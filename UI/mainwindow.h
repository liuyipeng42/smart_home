#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <string>
#include <QSocketNotifier>
#include "device.h"
#include "hc06.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui_;
    QLabel *title_;
    QPushButton *led_btn_;
    QPushButton *beep_btn_;
    QPushButton *door_btn_;

    QLabel *curtain_lable_;
    QProgressBar *curtain_degree_;
    QSlider *curtain_slider_;

    QLabel *IR_;    // 红外
    QLabel *IR_num_;
    QLabel *ALS_;   // 光照
    QLabel *ALS_num_;
    QLabel *PS_;    // 距离
    QLabel *PS_num_;

    QLabel *temperature_;
    QLabel *temperature_num_;
    QLabel *humidity_;
    QLabel *humidity_num_;
    QLabel *human_;
    QLabel *human_num_;
    QLabel *air_;
    QLabel *air_num_;

    Device *led_;
    Device *beep_;
    Device *door_;
    Device *curtain_;
    Device *ap3216c_;
    Device *dht11_;
    Device *sr501_;
    HC06 *hc06_;

    QSocketNotifier *socketNotifier_;

    int timer_cnt_ = 0;

private slots:
    void LEDBtnClicked();
    void BeepBtnClicked();
    void DoorBtnClicked();
    void CurtainSliderChanged();
    void Update();
    void sr501_handler();
    void reveive_from_hc06(char* device,  char*  value);
};

#endif // MAINWINDOW_H
