#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/types.h>
#include "ap3216creg.h"

#define DEV_CNT 1
#define DEV_NAME "ap3216c"

struct ap3216c_dev {
    dev_t devid;            // 设备号
    int major;              // 主设备号
    struct cdev cdev;       // cdev
    struct class* class;    // 类
    struct device* device;  // 设备
    struct i2c_client* client;
    unsigned short ir, als, ps;  // 三个光传感器数据
};

static struct ap3216c_dev ap3216cdev;

static int ap3216c_read_regs(struct ap3216c_dev* dev, u8 reg, void* val, int len) {
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client* client = dev->client;

    /* msg[0]为发送要读取的首地址 */
    msg[0].addr = client->addr; /* ap3216c地址 */
    msg[0].flags = 0;           /* 标记为发送数据 */
    msg[0].buf = &reg;          /* 读取的首地址 */
    msg[0].len = 1;             /* reg长度*/

    /* msg[1]读取数据 */
    msg[1].addr = client->addr; /* ap3216c地址 */
    msg[1].flags = I2C_M_RD;    /* 标记为读取数据*/
    msg[1].buf = val;           /* 读取数据缓冲区 */
    msg[1].len = len;           /* 要读取的数据长度*/

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret == 2) {
        ret = 0;
    } else {
        printk("i2c rd failed=%d reg=%06x len=%d\n", ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

static s32 ap3216c_write_regs(struct ap3216c_dev* dev, u8 reg, u8* buf, u8 len) {
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client* client = dev->client;

    b[0] = reg;              /* 寄存器首地址 */
    memcpy(&b[1], buf, len); /* 将要写入的数据拷贝到数组b里面 */

    msg.addr = client->addr; /* ap3216c地址 */
    msg.flags = 0;           /* 标记为写数据 */

    msg.buf = b;       /* 要写入的数据缓冲区 */
    msg.len = len + 1; /* 要写入的数据长度 */

    return i2c_transfer(client->adapter, &msg, 1);
}

static unsigned char ap3216c_read_reg(struct ap3216c_dev* dev, u8 reg) {
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
}

static void ap3216c_write_reg(struct ap3216c_dev* dev, u8 reg, u8 data) {
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

void ap3216c_readdata(struct ap3216c_dev* dev) {
    unsigned char i = 0;
    unsigned char buf[6];

    /* 循环读取所有传感器数据 */
    for (i = 0; i < 6; i++) {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    }

    if (buf[0] & 0X80) /* IR_OF位为1,则数据无效 */
        dev->ir = 0;
    else /* 读取IR传感器的数据   		*/
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

    dev->als = ((unsigned short)buf[3] << 8) | buf[2]; /* 读取ALS传感器的数据 			 */

    if (buf[4] & 0x40) /* IR_OF位为1,则数据无效 			*/
        dev->ps = 0;
    else /* 读取PS传感器的数据    */
        dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);
}

static int ap3216c_open(struct inode* inode, struct file* filp) {
    filp->private_data = &ap3216cdev;

    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04); /* 复位AP3216C 			*/
    mdelay(50);                                               /* AP3216C复位最少10ms 	*/
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0X03); /* 开启ALS、PS+IR 		*/
    return 0;
}

static ssize_t ap3216c_read(struct file* filp, char __user* buf, size_t cnt, loff_t* off) {
    short data[3];
    long err = 0;

    struct ap3216c_dev* dev = (struct ap3216c_dev*)filp->private_data;

    ap3216c_readdata(dev);

    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static const struct file_operations ap3216c_ops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read
};

static int ap3216c_probe(struct i2c_client* client, const struct i2c_device_id* id) {
    alloc_chrdev_region(&ap3216cdev.devid, 0, DEV_CNT, DEV_NAME);
    ap3216cdev.major = MAJOR(ap3216cdev.devid);

    cdev_init(&ap3216cdev.cdev, &ap3216c_ops);
    cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, DEV_CNT);

    ap3216cdev.class = class_create(THIS_MODULE, DEV_NAME);
    ap3216cdev.device = device_create(ap3216cdev.class, NULL, ap3216cdev.devid, NULL, DEV_NAME);

    ap3216cdev.client = client;

    return 0;
}

static int ap3216c_remove(struct i2c_client* client) {
    cdev_del(&ap3216cdev.cdev);
    unregister_chrdev_region(ap3216cdev.devid, DEV_CNT);
    device_destroy(ap3216cdev.class, ap3216cdev.devid);
    class_destroy(ap3216cdev.class);
    return 0;
}

static const struct i2c_device_id ap3216c_id[] = {{"alientek,ap3216c", 0}, {}};

static const struct of_device_id ap3216c_of_match[] = {{.compatible = "alientek,ap3216c"}, {/* Sentinel */}};

static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver =
        {
            .owner = THIS_MODULE,
            .name = DEV_NAME,
            .of_match_table = ap3216c_of_match,
        },
    .id_table = ap3216c_id,
};

static int __init ap3216c_init(void) {
    return i2c_add_driver(&ap3216c_driver);
}

static void __exit ap3216c_exit(void) {
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
