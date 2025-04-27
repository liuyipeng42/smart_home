#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/irqflags.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define DEV_CNT 1        /* 设备号长度 	*/
#define DEV_NAME "dht11" /* 设备名字 	*/

struct dht11_device {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct gpio_desc* dht11_gpio;
};

struct dht11_device dht11_dev;

static int read_dht11_data(u8* data) {
    u8 checksum = 0;

    gpiod_direction_output(dht11_dev.dht11_gpio, 0);
    mdelay(18);
    gpiod_direction_input(dht11_dev.dht11_gpio);
    udelay(40);

    if (gpiod_get_value(dht11_dev.dht11_gpio)) {
        printk(KERN_ERR "DHT11 sensor not responding\n");
        return 1;
    }

    udelay(80);

    if (!gpiod_get_value(dht11_dev.dht11_gpio)) {
        printk(KERN_ERR "DHT11 sensor not responding\n");
        return 1;
    }

    udelay(80);

    for (int i = 0; i < 5; i++) {
        data[i] = 0;
        for (int j = 7; j >= 0; j--) {
            u32 bit;

            while (!gpiod_get_value(dht11_dev.dht11_gpio))
                cpu_relax();

            udelay(50);

            bit = gpiod_get_value(dht11_dev.dht11_gpio);

            if (bit) {
                data[i] |= (1 << j);
                udelay(40);
            }
        }
    }

    checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        printk(KERN_ERR "DHT11 data checksum error\n");
        return 1;
    }

    return 0;
}

static ssize_t dht11_read(struct file* filp, char __user* buf, size_t size, loff_t* offset) {
    int res;
    unsigned long flags;

    u8 data[5] = {0};
    /* 因为DHT11的时序要求很高，所以在读温湿度的时候要让代码进入临界区，防止内核调度和抢占 */
    local_irq_save(flags);
    res = read_dht11_data(data);
    local_irq_restore(flags);

    res = copy_to_user(buf, data, size);
    return 0;
}

static struct file_operations dht11_ops = {
    .owner = THIS_MODULE,
    .read = dht11_read,
};

static int dht11_probe(struct platform_device* pdev) {
    alloc_chrdev_region(&dht11_dev.devid, 0, DEV_CNT, DEV_NAME);
    dht11_dev.major = MAJOR(dht11_dev.devid);

    cdev_init(&dht11_dev.cdev, &dht11_ops);
    cdev_add(&dht11_dev.cdev, dht11_dev.devid, DEV_CNT);

    dht11_dev.class = class_create(THIS_MODULE, DEV_NAME);
    dht11_dev.device = device_create(dht11_dev.class, NULL, dht11_dev.devid, NULL, DEV_NAME);

    dht11_dev.dht11_gpio = gpiod_get(&pdev->dev, DEV_NAME, GPIOD_OUT_HIGH);
    return 0;
}

static int dht11_remove(struct platform_device* dev) {
    gpiod_put(dht11_dev.dht11_gpio);

    cdev_del(&dht11_dev.cdev);
    unregister_chrdev(dht11_dev.major, DEV_NAME);
    device_destroy(dht11_dev.class, MKDEV(dht11_dev.major, 0));
    class_destroy(dht11_dev.class);
    return 0;
}

static const struct of_device_id dht11_of_match[] = {{.compatible = "dht11"}, {}};

static struct platform_driver dht11_driver = {
    .driver =
        {
            .name = DEV_NAME,
            .of_match_table = dht11_of_match,
        },
    .probe = dht11_probe,
    .remove = dht11_remove,
};

module_platform_driver(dht11_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LYP");
