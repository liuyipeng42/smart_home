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
#include <linux/pwm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define DEV_CNT 1       /* 设备号长度 	*/
#define DEV_NAME "sg90" /* 设备名字 	*/

struct sg90_device {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct pwm_device* pwm_dev;
};

struct sg90_device sg90_dev;

static ssize_t sg90_write(struct file* filp, const char __user* buf, size_t size, loff_t* offset) {
    int res;
    unsigned char data[1];
    if (size != 1)
        return 1;

    res = copy_from_user(data, buf, size);
    if (res < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    /* 配置PWM：旋转任意角度(单位1度) */
    pwm_config(sg90_dev.pwm_dev, 500000 + data[0] * 2000000 / 180, 20000000);
    return 1;
}

static struct file_operations sg90_ops = {
    .owner = THIS_MODULE,
    .write = sg90_write,
};

static int sg90_probe(struct platform_device* pdev) {
    alloc_chrdev_region(&sg90_dev.devid, 0, DEV_CNT, DEV_NAME);
    sg90_dev.major = MAJOR(sg90_dev.devid);

    cdev_init(&sg90_dev.cdev, &sg90_ops);
    cdev_add(&sg90_dev.cdev, sg90_dev.devid, DEV_CNT);

    sg90_dev.class = class_create(THIS_MODULE, DEV_NAME);
    sg90_dev.device = device_create(sg90_dev.class, NULL, sg90_dev.devid, NULL, DEV_NAME);

    // struct device_node* node = pdev->dev.of_node;
    // sg90_dev.pwm_dev = devm_of_pwm_get(&pdev->dev, node, NULL);
    sg90_dev.pwm_dev = pwm_get(&pdev->dev, "pwm-sg90"); /* 获取PWM设备 */

    pwm_set_polarity(sg90_dev.pwm_dev, PWM_POLARITY_NORMAL);  // 设置输出极性：占空比为高电平
    pwm_enable(sg90_dev.pwm_dev);                             // 使能PWM输出
    pwm_config(sg90_dev.pwm_dev, 500000, 20000000);           // 配置PWM：0.5ms，0度，周期：20000000ns = 20ms

    return 0;
}

static int sg90_remove(struct platform_device* dev) {
    pwm_disable(sg90_dev.pwm_dev);
    pwm_put(sg90_dev.pwm_dev);

    cdev_del(&sg90_dev.cdev);                          /*  删除cdev */
    unregister_chrdev_region(sg90_dev.devid, DEV_CNT); /* 注销设备号 */
    device_destroy(sg90_dev.class, sg90_dev.devid);
    class_destroy(sg90_dev.class);

    return 0;
}

static const struct of_device_id sg90_of_match[] = {{.compatible = "sg90"}, {}};

static struct platform_driver sg90_driver = {
    .driver =
        {
            .name = DEV_NAME,
            .of_match_table = sg90_of_match,
        },
    .probe = sg90_probe,
    .remove = sg90_remove,
};

module_platform_driver(sg90_driver);

MODULE_LICENSE("GPL");
