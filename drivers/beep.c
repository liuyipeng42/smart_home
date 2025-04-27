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

#define DEV_CNT 1       /* 设备号长度 	*/
#define DEV_NAME "beep" /* 设备名字 	*/

struct beep_device {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct gpio_desc* beep_gpio;
};

struct beep_device beep_dev;  // beep设备

static ssize_t beep_write(struct file* filp, const char __user* buf, size_t cnt, loff_t* offt) {
    int retvalue;
    unsigned char databuf[2];

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) {
        printk("kernel write faibeep!\r\n");
        return -EFAULT;
    }

    gpiod_set_value(beep_dev.beep_gpio, databuf[0]);
    return 0;
}

static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .write = beep_write,
};

static int beep_probe(struct platform_device* dev) {
    alloc_chrdev_region(&beep_dev.devid, 0, DEV_CNT, DEV_NAME);
    beep_dev.major = MAJOR(beep_dev.devid);

    cdev_init(&beep_dev.cdev, &beep_fops);
    cdev_add(&beep_dev.cdev, beep_dev.devid, DEV_CNT);

    beep_dev.class = class_create(THIS_MODULE, DEV_NAME);
    beep_dev.device = device_create(beep_dev.class, NULL, beep_dev.devid, NULL, DEV_NAME);

    beep_dev.beep_gpio = gpiod_get(&dev->dev, DEV_NAME, GPIOD_OUT_LOW);
    return 0;
}

static int beep_remove(struct platform_device* dev) {
    gpiod_set_value(beep_dev.beep_gpio, 0);  // 关闭 beep
    gpiod_put(beep_dev.beep_gpio);           // 释放 descriptor

    cdev_del(&beep_dev.cdev);                          /*  删除cdev */
    unregister_chrdev_region(beep_dev.devid, DEV_CNT); /* 注销设备号 */
    device_destroy(beep_dev.class, beep_dev.devid);
    class_destroy(beep_dev.class);
    return 0;
}

/* 匹配列表 */
static const struct of_device_id beep_of_match[] = {{.compatible = "beep"}, {/* Sentinel */}};

/* platform驱动结构体 */
static struct platform_driver beep_driver = {
    .driver =
        {
            .name = DEV_NAME,                /* 驱动名字，用于和设备匹配 */
            .of_match_table = beep_of_match, /* 设备树匹配表 		 */
        },
    .probe = beep_probe,
    .remove = beep_remove,
};

module_platform_driver(beep_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LYP");
