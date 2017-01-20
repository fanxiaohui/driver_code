/*
 *  dac081s101 , tc32163fg & ad747x driver
 *  Linux kernel module for
 * 	Texas Instruments RF & TOSHIBA TC32163FG
 *
 *  Copyright (c) 2013 Guanghong Xu <xughg@gevnict.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <plat/omap-spi.h>

#define DAC081S101_DRV_NAME	"dac081s101"
#define TC32163FG_DRV_NAME	"tc32163fg"
#define AD7478_DRV_NAME     "ad7478"
#define CYCLONE_PS_DRV_NAME "cyclone_ps"
#define DRIVER_VERSION		"1.0.5"
/*
 * 1.0.2 add ad7478 driver
 * 1.0.3 add cyclone ps interface driver
 */
#define SPI0_MAJOR		        153
#define SPI_GPIO_MAJOR		    154
#define LM75_MAJOR              152

#define CS_DAC      0
#define CS_TB       4
#define CS_ADC      3

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;
    struct cdev cdev;

	/* buffer is NULL unless this device is open (users > 0) */
	struct mutex	buf_lock;
	unsigned		users;
	u8				*buffer;
};

//���ڸ�SPI�����ϵ��豸����
static LIST_HEAD(device_list);
//��������
static DEFINE_MUTEX(device_list_lock);

//��Ƶ�豸��
static struct class *rf_class;

//ÿ��SPI�豸�Ļ�������С
static int bufsiz = 4096;

/* �ն˵ķ�ʽд�豸 ? ��echo 1234 > /sys/class/rf_class/dac081s101/value��*/
static ssize_t dac081s101_store_val(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	unsigned char tmp[2];
	unsigned long val;

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;

	tmp[0] = val >> 4;
	tmp[1] = val & 0xf0;
	dev_info(&spi->dev, "write: %02x %02x\n",
			tmp[0], tmp[1]);
	gpio_set_value(pdata->gpio_cs, 0);
	spi_write(spi, tmp, sizeof(tmp));
	gpio_set_value(pdata->gpio_cs, 1);

	return count;
}

//�豸�ն�д����
static DEVICE_ATTR(dac, S_IWUSR, NULL, dac081s101_store_val);

//�豸�ն�д����
static struct attribute *dac081s101_attributes[] = {
	&dev_attr_dac.attr,
	NULL
};

//�豸�ն�д����
static const struct attribute_group dac081s101_attr_group = {
	.attrs = dac081s101_attributes,
};

/* �ն˵ķ�ʽд�豸 ? ��echo 1234 > /sys/class/rf_class/tc32163fg/value��*/
static ssize_t tc32163fg_store_val(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{

	struct spi_device *spi = to_spi_device(dev);
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	unsigned long val;
	u8 buff[9];
    u32 i;
    u32 d, n ,s;
    int ret = 0;
    d = n = s =0;


	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;

    d = 0;
    n = 0x7578D * 4 + 3;
    s = 0x104   * 4 + 1;
	buff[0] = (u8)((d >> 16) & 0xff);
	buff[1] = (u8)((d >>  8) & 0xff);
	buff[2] = (u8)((d >>  0) & 0xff);
	buff[3] = (u8)((n >> 16) & 0xff);
	buff[4] = (u8)((n >>  8) & 0xff);
	buff[5] = (u8)((n >>  0) & 0xff);
	buff[6] = (u8)((s >> 16) & 0xff);
	buff[7] = (u8)((s >>  8) & 0xff);
	buff[8] = (u8)((s >>  0) & 0xff);
	pr_info("spi_write: testing...\n");

    for (i=0; i<3; ++i) {
        gpio_set_value(pdata->gpio_cs, 0);
        pr_info("%02x %02x %02x\n", buff[0+i*3], buff[1+i*3], buff[2+i*3]);
        ret = spi_write(spi, buff + i * 3, 3);
        if (ret < 0) {
		    dev_err(&spi->dev, "SPI write word error\n");
		    return ret;
	    }
        gpio_set_value(pdata->gpio_cs, 1);
	    mdelay(1);
	}
	gpio_set_value(pdata->gpio_cs, 0);

	pr_info("spi_write: sending finish\n");

	return count;
}

//�豸�ն�д����
static DEVICE_ATTR(tc, S_IWUSR, NULL, tc32163fg_store_val);

//�豸�ն�д����
static struct attribute *tc32163fg_attributes[] = {
	&dev_attr_tc.attr,
	NULL
};

//�豸�ն�д����
static const struct attribute_group tc32163fg_attr_group = {
	.attrs = tc32163fg_attributes,
};

#if 1
/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void spidev_complete(void *arg)
{
	complete(arg);
}

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = spidev_complete;
	message->context = &done;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
	spin_unlock_irq(&spidev->spi_lock);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}

static inline ssize_t
spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}
/* Write-only message with current device setup */
static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}
/*-------------------------------------------------------------------------*/
/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev= filp->private_data;
    struct spi_device *spi = spidev->spi;
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	ssize_t status  = 0;


	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;


	mutex_lock(&spidev->buf_lock);

    if ((pdata != NULL) && (pdata->gpio_cs))
	    gpio_set_value(pdata->gpio_cs, 0);
    status = spidev_sync_read(spidev, count);
    if ((pdata != NULL) && (pdata->gpio_cs))
        gpio_set_value(pdata->gpio_cs, 1);

	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, spidev->buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev = filp->private_data;
	struct spi_device *spi = spidev->spi;
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	ssize_t			status = 0;
	unsigned long	missing;


	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		count = bufsiz;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->buffer, buf, count);
	if (missing == 0) {
        if ((pdata != NULL) && (pdata->gpio_cs))
    		gpio_set_value(pdata->gpio_cs, 0);
        status = spidev_sync_write(spidev, count);
        if (spi->chip_select == CS_TB) {    // fix for tc32163fg
            if ((pdata != NULL) && (pdata->gpio_cs)) {
                gpio_set_value(pdata->gpio_cs, 1);
                gpio_set_value(pdata->gpio_cs, 0);
            }
        } else if ((pdata != NULL) && (pdata->gpio_cs)){
            gpio_set_value(pdata->gpio_cs, 1);
        }
	} else
		status = -EFAULT;
	mutex_unlock(&spidev->buf_lock);

	return status;
}
#else
/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev = filp->private_data;
	struct spi_device *spi = spidev->spi;
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	int 			ret;
	unsigned long	missing;

	if (count > bufsiz)
		count = bufsiz;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->buffer, buf, count);

	if (missing == 0) {
        if ((pdata != NULL) && (pdata->gpio_cs))
    		gpio_set_value(pdata->gpio_cs, 0);
        ret = spi_write(spi, spidev->buffer, count);
        if (spi->chip_select == CS_TB) {    // fix for tc32163fg
            if ((pdata != NULL) && (pdata->gpio_cs)) {
                gpio_set_value(pdata->gpio_cs, 1);
                gpio_set_value(pdata->gpio_cs, 0);
            }
        } else if ((pdata != NULL) && (pdata->gpio_cs)){
            gpio_set_value(pdata->gpio_cs, 1);
        }

        if (ret < 0) {
		    dev_err(&spi->dev, "spi write word error\n");
		    ret = -EFAULT;
	    } else
	        ret = count;
	} else
		ret = -EFAULT;

	mutex_unlock(&spidev->buf_lock);

	return ret;
}

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev = filp->private_data;
    struct spi_device *spi = spidev->spi;
	struct omap_spi_platform_data *pdata = spi->dev.platform_data;
	ssize_t status  = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	mutex_lock(&spidev->buf_lock);

    if ((pdata != NULL) && (pdata->gpio_cs))
	    gpio_set_value(pdata->gpio_cs, 0);
    status = spi_read(spi,  spidev->buffer, count);
    if ((pdata != NULL) && (pdata->gpio_cs))
        gpio_set_value(pdata->gpio_cs, 1);

	if (status < 0) {
        dev_err(&spi->dev, "spi read word error:%d\n", status);
	    status = -EFAULT;
	} else {
		unsigned long	missing;

		missing = copy_to_user(buf, spidev->buffer, count);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
    }

	mutex_unlock(&spidev->buf_lock);

	return status;
}
#endif
/*****************************************************************
 �������ƣ�rf_open
 ��������������Ƶ��������豸�ӿ�
 ���������inode,�豸�ڵ�
		   filp���ļ�����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int rf_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) { // VVVVVVVVVVVVVVVVV
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (!spidev->buffer) {
			spidev->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (!spidev->buffer) {
				dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			spidev->users++;
			filp->private_data = spidev;
			nonseekable_open(inode, filp);
		}
	} else
        dev_err(&spidev->spi->dev, "nothing for minor %d\n", iminor(inode));

	mutex_unlock(&device_list_lock);
	return status;
}

/*****************************************************************
 �������ƣ�rf_release
�����������ر���Ƶ�����豸�ӿ�
 ���������inode,�豸�ڵ�
		   filp���ļ�����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int rf_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = 0;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	spidev->users--;
	if (!spidev->users) {
		int		dofree;

		kfree(spidev->buffer);
		spidev->buffer = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&spidev->spi_lock);
		dofree = (spidev->spi == NULL);
		spin_unlock_irq(&spidev->spi_lock);

		if (dofree)
			kfree(spidev);
	}
	mutex_unlock(&device_list_lock);

	return status;
}

//dac081s101 DACת��оƬ�ļ�����
static const struct file_operations dac081s101_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	// .read =		tc32163fg_read,
	// .unlocked_ioctl = tc32163fg_ioctl,
	.open =		rf_open,
	.release =	rf_release,
	.llseek =	no_llseek,
};

//tc32163fg 5.8G��ƵоƬ�ļ�����
static const struct file_operations tc32163fg_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	// .read =		tc32163fg_read,
	// .unlocked_ioctl = tc32163fg_ioctl,
	.open =		rf_open,
	.release =	rf_release,
	.llseek =	no_llseek,
};

//ad7478 ADCת��оƬ�ļ�����
static const struct file_operations ad7478_fops = {
	.owner =	THIS_MODULE,
	//.write =	rf_write,
	 .read =	spidev_read,
	// .unlocked_ioctl = tc32163fg_ioctl,
	.open =		rf_open,
	.release =	rf_release,
	.llseek =	no_llseek,
};

//FPGA FPGA���������ӿڵ��ļ�����
static const struct file_operations cyclone_ps_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	 //.read =	spidev_read,
	// .unlocked_ioctl = tc32163fg_ioctl,
	.open =		rf_open,
	.release =	rf_release,
	.llseek =	no_llseek,
};

/*****************************************************************
 �������ƣ�cdev_setup
 ������������ʼ���ַ��豸�������豸�ļ�
 ���������spidev���豸˽������
		major���ַ��豸���豸��
		minor���ַ��豸���豸��
		fops���ַ��豸�ļ�����
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static int cdev_setup(struct spidev_data *spidev, int major,
                       int minor, const struct file_operations *fops)
{
    struct cdev *cdev = &spidev->cdev;
    struct device *dev;
    int err;
    dev_t devt = MKDEV(major, minor);

    cdev_init(cdev, fops);
    cdev->owner = THIS_MODULE;
    cdev->ops = fops;

    err = cdev_add(cdev, devt, 1);
    if (err) {
        printk(KERN_ERR "add %s cdev error\n", spidev->spi->modalias);
        return err;
    }

    dev = device_create(rf_class, &spidev->spi->dev, devt, spidev, "%s", spidev->spi->modalias);
    err = IS_ERR(dev) ? PTR_ERR(dev) : 0;
    if (!err)
        spidev->devt = devt;
    else {
        cdev_del(cdev);
        printk(KERN_ERR "%s device_create fail\n", spidev->spi->modalias);
    }

    return err;
}

/*****************************************************************
 �������ƣ�cdev_release
 ����������ע���ַ��豸��ɾ���豸�ļ�
 ���������spidev���豸˽������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void cdev_release(struct spidev_data	*spidev)
{
    cdev_del(&spidev->cdev);

    if (spidev->devt) {
        device_destroy(rf_class, spidev->devt);
    }
}

/*****************************************************************
 �������ƣ�rf_probe
 �����������豸̽�⺯�����豸��������Ժ�̽���ܷ���������
 ���������pdev���豸����
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int __devinit rf_probe(struct spi_device *spi)
{
	struct  spidev_data	*spidev;
	int	    status, ret;

	dev_info(&spi->dev, "driver initializing\n");
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret < 0)
		return ret;
	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/*
	 * major is SPI0_MAJOR, minior is spi->chip_select.
	 */
    /*
	mutex_lock(&device_list_lock);
    if (!strcmp(spidev->spi->modalias, "dac081s101")) {
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &dac081s101_fops);
        status |= sysfs_create_group(&spi->dev.kobj, &dac081s101_attr_group);
    } else if (!strcmp(spidev->spi->modalias, "tc32163fg")) {
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &tc32163fg_fops);
        status |= sysfs_create_group(&spi->dev.kobj, &tc32163fg_attr_group);
    } else if (!strcmp(spidev->spi->modalias, "ad7478")) {
        sprintf(spidev->spi->modalias, "ad7478.%d", spi->chip_select-2);
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &ad7478_fops);
        sprintf(spidev->spi->modalias, "ad7478");
    } else if (!strcmp(spidev->spi->modalias, "cyclone_ps")) {
        status  = cdev_setup(spidev, SPI_GPIO_MAJOR, spi->chip_select, &cyclone_ps_fops);
    }
    else {
        printk(KERN_ERR "unknow spi device %s\n", spidev->spi->modalias);
        status = -EFAULT;
    }
    */
    printk(KERN_INFO "-------cdev_setup for %s\n",spidev->spi->modalias);
    mutex_lock(&device_list_lock);
    if (!strcmp(spidev->spi->modalias, "dac081s101")) {
        const char *dac_user_str[] = {"dac081s101.rf_power","dac081s101.rf_sens","dac081s101.gain"};
        sprintf(spidev->spi->modalias, "%s", dac_user_str[spi->chip_select]);
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &dac081s101_fops);
        sprintf(spidev->spi->modalias, "dac081s101");
        //status |= sysfs_create_group(&spi->dev.kobj, &dac081s101_attr_group);
    } else if (!strcmp(spidev->spi->modalias, "tc32163fg")) {
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &tc32163fg_fops);
        //status |= sysfs_create_group(&spi->dev.kobj, &tc32163fg_attr_group);
    } else if (!strcmp(spidev->spi->modalias, "ad7478")) {
        status  = cdev_setup(spidev, SPI0_MAJOR, spi->chip_select, &ad7478_fops);
    } else if (!strcmp(spidev->spi->modalias, "cyclone_ps")) {
        status  = cdev_setup(spidev, SPI_GPIO_MAJOR, spi->chip_select, &cyclone_ps_fops);
    }
    else {
        printk(KERN_ERR "unknow spi device %s\n", spidev->spi->modalias);
        status = -EFAULT;
    }

	if (status == 0)
		list_add(&spidev->device_entry, &device_list);
	mutex_unlock(&device_list_lock);

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);

	return status;
}

/*****************************************************************
 �������ƣ�rf_remove
 ����������ע����Ƶ����豸����Դ
 ���������pin_mux���������Խ��
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static int __devexit rf_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

    dev_info(&spi->dev, "driver remove\n");

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);

    /*
    if (!strcmp(spi->modalias, "dac081s101"))
        sysfs_remove_group(&spi->dev.kobj, &dac081s101_attr_group);
    else if (!strcmp(spi->modalias, "tc32163fg"))
        sysfs_remove_group(&spi->dev.kobj, &tc32163fg_attr_group);

    if (strstr(spi->modalias, "dac081s101") != NULL)
        sysfs_remove_group(&spi->dev.kobj, &dac081s101_attr_group);
    else if (!strcmp(spi->modalias, "tc32163fg"))
        sysfs_remove_group(&spi->dev.kobj, &tc32163fg_attr_group);
    */

	if (spidev->users == 0) {
        cdev_release(spidev);
		kfree(spidev);
    }
	mutex_unlock(&device_list_lock);

	return 0;
}

//dac081s101 DACоƬ������
static struct spi_driver dac081s101_driver = {
	.driver = {
		.name	= "dac081s101",
		.owner	= THIS_MODULE,
	},
	.probe	= rf_probe,
	.remove	= __devexit_p(rf_remove),
};

//tc32163fg ��ƵоƬ������
static struct spi_driver tc32163fg_driver = {
    .driver = {
		.name	= "tc32163fg",
		.owner	= THIS_MODULE,
	},
	.probe	= rf_probe,
	.remove	= __devexit_p(rf_remove),
};

//ad7478 ADCоƬ������
static struct spi_driver ad7478_driver = {
    .driver = {
		.name	= "ad7478",
		.owner	= THIS_MODULE,
	},
	.probe	= rf_probe,
	.remove	= __devexit_p(rf_remove),
};

//FPGA FPGAоƬ�����ӿڵ�����
static struct spi_driver cyclone_ps_driver = {
    .driver = {
		.name	= "cyclone_ps",
		.owner	= THIS_MODULE,
	},
	.probe	= rf_probe,
	.remove	= __devexit_p(rf_remove),
};

/*****************************************************************
 �������ƣ�rf_init
 ����������ģ�����ʱ��ʼ��
 �����������
 �����������
 ����˵�����ɹ���ʶ
 ����˵����
 *****************************************************************/
static int __init rf_init(void)
{
	int status;

    printk(KERN_INFO "spi driver insert, driver version: %s\n", DRIVER_VERSION);
	rf_class = class_create(THIS_MODULE, "spi_slave");
	if (IS_ERR(rf_class)) {
		printk(KERN_ERR "%s: create class error\n", __func__);
		return PTR_ERR(rf_class);
	}

	printk(KERN_INFO "------------------------------------------%s\n",__FUNCTION__);
	status = spi_register_driver(&dac081s101_driver);
	if (status < 0) {
		printk(KERN_ERR "%s: dac081s101_driver register  error\n", __func__);
        goto dac081s101_err;
	}
    status = spi_register_driver(&ad7478_driver);
    if (status < 0) {
        printk(KERN_ERR "%s: ad7478_driver register  error\n", __func__);
        goto ad7478_err;
    }
    status = spi_register_driver(&tc32163fg_driver);
	if (status < 0) {
		printk(KERN_ERR "%s: tc32163fg_driver register  error\n", __func__);
        goto tc32163fg_err;
	}
    status = spi_register_driver(&cyclone_ps_driver);
    if (status < 0) {
        printk(KERN_ERR "%s: cyclone_ps_driver register  error\n", __func__);
        goto cyclone_ps_err;
    }

    return 0;

cyclone_ps_err:
    spi_unregister_driver(&cyclone_ps_driver);
tc32163fg_err:
    spi_unregister_driver(&tc32163fg_driver);
ad7478_err:
    spi_unregister_driver(&ad7478_driver);
dac081s101_err:
    spi_unregister_driver(&dac081s101_driver);
    class_destroy(rf_class);

	return status;
}

/*****************************************************************
 �������ƣ�rf_exit
 ����������ģ��ע��ʱ�ͷ���Դ��
 �����������
 �����������
 ����˵������
 ����˵����
 *****************************************************************/
static void __exit rf_exit(void)
{
    printk(KERN_INFO "spi driver remove\n");

    spi_unregister_driver(&ad7478_driver);
    spi_unregister_driver(&tc32163fg_driver);
    spi_unregister_driver(&dac081s101_driver);
    spi_unregister_driver(&cyclone_ps_driver);

	class_destroy(rf_class);
}

MODULE_AUTHOR("Guanghong Xu <xughg@genvict.com>");
MODULE_DESCRIPTION("DAC081S101, TC32163FG, AD7478 & CYCLONE_PS driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(rf_init);
module_exit(rf_exit);

