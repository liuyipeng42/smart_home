# Smart Home

## Overview

基于 **I.MX6ULL** 平台的智能家居项目，支持以下外设驱动：

- LED 灯
- 扬声器
- 继电器
- SG90 舵机
- AP3216C 传感器
- DHT11 温湿度传感器
- SR501 人体感应模块

此外，项目支持使用 **HC-06 蓝牙模块** 控制 LED、扬声器、继电器和 SG90 舵机。

## How to Build

1. 克隆仓库：
    ```bash
    git clone https://github.com/yourusername/smart_home.git
    cd smart_home
    ```

2. 配置 I.MX6ULL 平台的交叉编译环境，并准备 ARM 版 Qt。

3. 编译项目：
    ```bash
    make
    cd UI
    ./build.sh
    ```

## Notes

- 请确保在编译前正确安装交叉编译工具链和相关依赖。
- `UI` 目录下包含基于 Qt 的用户界面程序，需单独构建。
- 若需通过蓝牙控制，请预先完成 HC-06 配对，并确保串口连接正确。
