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

#define DEV_CNT 1         // 设备号长度
#define DEV_NAME "relay"  // 设备名字

struct relay_device {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct gpio_desc* relay0_gpio;
};

struct relay_device relay_dev;  // relay设备


static ssize_t relay_write(struct file* filp, const char __user* buf, size_t cnt, loff_t* offt) {
    int retvalue;
    unsigned char databuf[2];

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0) {
        printk("kernel write fairelay!\r\n");
        return -EFAULT;
    }

    gpiod_set_value(relay_dev.relay0_gpio, databuf[0]);
    return 0;
}

/* 设备操作函数 */
static struct file_operations relay_fops = {
    .owner = THIS_MODULE,
    .write = relay_write,
};

static int relay_probe(struct platform_device* dev) {
    alloc_chrdev_region(&relay_dev.devid, 0, DEV_CNT, DEV_NAME);
    relay_dev.major = MAJOR(relay_dev.devid);

    cdev_init(&relay_dev.cdev, &relay_fops);
    cdev_add(&relay_dev.cdev, relay_dev.devid, DEV_CNT);

    relay_dev.class = class_create(THIS_MODULE, DEV_NAME);
    relay_dev.device = device_create(relay_dev.class, NULL, relay_dev.devid, NULL, DEV_NAME);

    relay_dev.relay0_gpio = gpiod_get(&dev->dev, DEV_NAME, GPIOD_OUT_LOW);
    return 0;
}

static int relay_remove(struct platform_device* dev) {
    gpiod_set_value(relay_dev.relay0_gpio, 0);  // 关闭 relay
    gpiod_put(relay_dev.relay0_gpio);           // 释放 descriptor

    cdev_del(&relay_dev.cdev);                          /*  删除cdev */
    unregister_chrdev_region(relay_dev.devid, DEV_CNT); /* 注销设备号 */
    device_destroy(relay_dev.class, relay_dev.devid);
    class_destroy(relay_dev.class);
    return 0;
}

static const struct of_device_id relay_of_match[] = {{.compatible = "relay"}, {/* Sentinel */}};

static struct platform_driver relay_driver = {
    .driver =
        {
            .name = DEV_NAME,                 /* 驱动名字，用于和设备匹配 */
            .of_match_table = relay_of_match, /* 设备树匹配表 		 */
        },
    .probe = relay_probe,
    .remove = relay_remove,
};

module_platform_driver(relay_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LYP");
