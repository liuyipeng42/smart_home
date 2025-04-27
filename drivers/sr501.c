#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define DEV_CNT 1        /* 设备号长度 */
#define DEV_NAME "sr501" /* 设备名字 */

struct sr501_device {
    dev_t devid;
    struct cdev cdev;
    struct class* class;
    struct device* device;
    struct gpio_desc* sr501_gpio;

    int irq;
    struct timer_list timer;

    wait_queue_head_t wait_queue;  // 新增等待队列
    bool data_ready;               // 数据就绪标志
};

static struct sr501_device sr501_dev;  // 静态分配设备结构体

static void timeout_handler(unsigned long data) {
    sr501_dev.data_ready = true;  // 重置数据就绪标志
    // 唤醒等待队列，被唤醒的进程会重新执行驱动的 .poll 函数
    wake_up_interruptible(&sr501_dev.wait_queue);
}

static irqreturn_t sr501_handler(int irq, void* dev_id) {
    mod_timer(&sr501_dev.timer, jiffies + HZ / 50);
    return IRQ_HANDLED;
}

static ssize_t sr501_read(struct file* filp, char __user* buf, size_t size, loff_t* offset) {
    char val;
    int ret;

    if (size < 1)
        return -EINVAL;

    val = gpiod_get_value(sr501_dev.sr501_gpio);
    ret = copy_to_user(buf, &val, 1);
    if (ret)
        return -EFAULT;

    return 1;
}

static unsigned int sr501_poll(struct file* filp, poll_table* wait) {
    unsigned int mask = 0;
    // poll_table 与 调用poll的进程 关联，poll_wait 会将没有添加过的进程添加到等待队列中
    poll_wait(filp, &sr501_dev.wait_queue, wait);
    if (sr501_dev.data_ready) {
        mask |= POLLIN;                // 标记数据可读
        sr501_dev.data_ready = false;  // 重置标志（可选）
    }
    return mask;
}

static const struct file_operations sr501_ops = {
    .owner = THIS_MODULE,
    .read = sr501_read,
    .poll = sr501_poll,
};

static int sr501_probe(struct platform_device* pdev) {
    int gpio_num;
    int ret;

    alloc_chrdev_region(&sr501_dev.devid, 0, DEV_CNT, DEV_NAME);

    cdev_init(&sr501_dev.cdev, &sr501_ops);
    cdev_add(&sr501_dev.cdev, sr501_dev.devid, DEV_CNT);
    sr501_dev.class = class_create(THIS_MODULE, DEV_NAME);
    sr501_dev.device = device_create(sr501_dev.class, NULL, sr501_dev.devid, NULL, DEV_NAME);

    sr501_dev.sr501_gpio = gpiod_get(&pdev->dev, DEV_NAME, GPIOD_IN);
    gpio_num = desc_to_gpio(sr501_dev.sr501_gpio);
    sr501_dev.irq = gpio_to_irq(gpio_num);

    ret = request_irq(sr501_dev.irq, sr501_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "sr501_irq",
                      NULL);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
    }

    setup_timer(&sr501_dev.timer, timeout_handler, 0);

    init_waitqueue_head(&sr501_dev.wait_queue);
    sr501_dev.data_ready = false;

    return 0;
}

static int sr501_remove(struct platform_device* pdev) {
    del_timer_sync(&sr501_dev.timer);
    free_irq(sr501_dev.irq, NULL);
    gpiod_put(sr501_dev.sr501_gpio);

    device_destroy(sr501_dev.class, sr501_dev.devid);
    class_destroy(sr501_dev.class);
    cdev_del(&sr501_dev.cdev);

    unregister_chrdev_region(sr501_dev.devid, DEV_CNT);

    return 0;
}

static const struct of_device_id sr501_of_match[] = {{.compatible = "sr501"}, {}};

static struct platform_driver sr501_platform_driver = {
    .driver =
        {
            .name = DEV_NAME,
            .of_match_table = sr501_of_match,
        },
    .probe = sr501_probe,
    .remove = sr501_remove,
};

module_platform_driver(sr501_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LYP");
