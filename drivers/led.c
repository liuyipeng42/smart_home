#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/wait.h>

#define DEV_CNT 1      /* 设备号长度 	*/
#define DEV_NAME "led" /* 设备名字 	*/

struct led_device {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct gpio_desc* led0_gpio;
};

struct led_device led_dev;  // led设备

static ssize_t led_write(struct file* filp, const char __user* buf, size_t cnt, loff_t* offt) {
    int res;
    unsigned char databuf[2];

    res = copy_from_user(databuf, buf, cnt);
    if (res < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    gpiod_set_value(led_dev.led0_gpio, databuf[0]);
    return 0;
}

/* 设备操作函数 */
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
};

static int led_probe(struct platform_device* dev) {
    // 1、设置设备号 
    alloc_chrdev_region(&led_dev.devid, 0, DEV_CNT, DEV_NAME);
    led_dev.major = MAJOR(led_dev.devid);

    // 2、注册设备
    cdev_init(&led_dev.cdev, &led_fops);
    cdev_add(&led_dev.cdev, led_dev.devid, DEV_CNT);

    // 3、创建类 /sys/class/DEV_NAME
    // 4、创建设备 /dev/DEV_NAME
    led_dev.class = class_create(THIS_MODULE, DEV_NAME);
    led_dev.device = device_create(led_dev.class, NULL, led_dev.devid, NULL, DEV_NAME);

    led_dev.led0_gpio = gpiod_get(&dev->dev, DEV_NAME, GPIOD_OUT_LOW);
    return 0;
}

static int led_remove(struct platform_device* dev) {
    gpiod_set_value(led_dev.led0_gpio, 0);  // 关闭 LED
    gpiod_put(led_dev.led0_gpio);           // 释放 descriptor

    cdev_del(&led_dev.cdev);                          /*  删除cdev */
    unregister_chrdev_region(led_dev.devid, DEV_CNT); /* 注销设备号 */
    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class);
    return 0;
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {{.compatible = "led"}, {/* Sentinel */}};

/* platform驱动结构体 */
static struct platform_driver led_driver = {
    .driver =
        {
            .name = DEV_NAME,               /* 驱动名字，用于和设备匹配 */
            .of_match_table = led_of_match, /* 设备树匹配表 		 */
        },
    .probe = led_probe,
    .remove = led_remove,
};

module_platform_driver(led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LYP");
