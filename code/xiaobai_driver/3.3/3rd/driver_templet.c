/*		
 *	name:	waitqueuqe_test.c
 *	auth:	wuw
 *	date:	2016��1��30�� 18:47:31
 *	
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>  
 
#include <linux/kernel.h>  
 
#include <linux/string.h>  
#include <linux/sysfs.h>  
#include <linux/stat.h> 
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>

#include <linux/delay.h>



#define DEBUG_SWITCH    1
#if DEBUG_SWITCH
	#define P_DEBUG(fmt, args...)   printk("<1>" "<kernel>[%s]"fmt, __FUNCTION__, ##args)
#else
	#define P_DEBUG(fmt, args...)   printk("<7>" "<kernel>[%s]"fmt, __FUNCTION__, ##args)
#endif
#define DEV_SIZE 100

struct my_dev_t
{
	char kbuf[DEV_SIZE];
	dev_t devno;
	struct cdev cdev;
	struct class *cls;
	struct device *dev;
	unsigned int cur_size;

};


struct my_dev_t *my_dev;
static int dev_major = 0;


int my_open(struct inode *inode, struct file *filp)
{
	struct my_dev_t *dev;
	dev = container_of(inode->i_cdev, struct my_dev_t, cdev);
	filp->private_data = dev;
	return 0;

}





int my_close(struct inode *node, struct file *filp)
{
	return 0;
}

ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	int ret;
	struct my_dev_t *dev = filp->private_data;

	if(*offset >= DEV_SIZE){//���ƫ�����Ѿ����������������
		return count ? - ENXIO : 0;	//countΪ0�򷵻�0����ʾ��ȡ0�����ݳɹ�
	}								//count��Ϊ0��ֻ����ţ���ַԽ��
	if(*offset + count > DEV_SIZE){	//�����ȡ�ֽ������������ƫ����
		count = DEV_SIZE - *offset;	//����ٶ�ȡ�ֽ�����
	}
	/*copy_to_user�Ĳ���ҲҪ��һ��*/
	if (copy_to_user(buf, dev->kbuf + *offset, count)){
		ret = - EFAULT;
	}else{
		ret = count;
		dev->cur_size -= count;	//��ȡ��������ֽ�������
		*offset += count;		//ƫ��������
		P_DEBUG("read %d bytes, cur_size:[%d]\n", count, dev->cur_size);
	}

	return ret;		//����ʵ��д����ֽ���������
}

ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	int ret;
	struct my_dev_t *dev = filp->private_data;

	if(*offset >= DEV_SIZE){//���ƫ�����Ѿ����������������
		return count ? - ENXIO : 0;	//countΪ0�򷵻�0����ʾ��ȡ0�����ݳɹ�
	}								//count��Ϊ0��ֻ����ţ���ַԽ��
	if(*offset + count > DEV_SIZE){	//�����ȡ�ֽ������������ƫ����
		count = DEV_SIZE - *offset;	//����ٶ�ȡ�ֽ�����
	}
	/*copy_from_user�Ĳ���ҲҪ��һ��*/
	if(copy_from_user(dev->kbuf + *offset, buf, count)){
		ret = - EFAULT;
	}else{
		ret = count;
		dev->cur_size += count;	//д���������ֽ�������
		*offset += count;		//ƫ��������
		P_DEBUG("write %d bytes, cur_size:[%d]\n", count, dev->cur_size);
		P_DEBUG("kbuf is [%s]\n", dev->kbuf);
	}

	return ret;		//����ʵ��д����ֽ���������
}

loff_t my_llseek (struct file *filp, loff_t offset, int whence)
{
	loff_t new_pos;					//��ƫ����
	loff_t old_pos = filp->f_pos;	//��ƫ����

	switch(whence){
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = old_pos + offset;
			break;
		case SEEK_END:
			new_pos = DEV_SIZE + offset;
			break;
		default:
			P_DEBUG("unknow whence\n");
			return - EINVAL;
	}

	if(new_pos < 0 || new_pos > DEV_SIZE){	//���ƫ����Խ�磬���ش����
		P_DEBUG("f_pos failed\n");
		return - EINVAL;
	}

	filp->f_pos = new_pos;
	return new_pos;							//��ȷ�����µ�ƫ����
}



static const struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.llseek = my_llseek,
	.release = my_close,
};



static int __init test_init(void)
{
	int ret = 0;
	
	my_dev = kmalloc(sizeof(struct my_dev_t), GFP_KERNEL);
	if(my_dev == NULL){
		printk("Err, init kmalloc failed!\n");
		return -1;
	}
	memset(my_dev, 0, sizeof(struct my_dev_t));
	/*�����豸�źʹ��豸�ŵõ��豸��, ע���豸��*/
	my_dev->devno = MKDEV(dev_major, 0);
	if(dev_major)
		ret = register_chrdev_region(my_dev->devno, 1, "my_dev");
	else{
		ret = alloc_chrdev_region(&my_dev->devno, 0, 1, "my_dev");
		dev_major = MAJOR(my_dev->devno);
	}
	if(ret < 0)
		goto register_chrdev_error;

	/*���cdev*/
	cdev_init(&my_dev->cdev, &fops);
	ret = cdev_add(&my_dev->cdev, my_dev->devno, 1);
	if(ret < 0){
		printk("Err, init cdev_add failed!\n");
		goto cdev_add_error;
	}

	/*����class*/
	my_dev->cls = class_create(THIS_MODULE, "my_class");
	/*��IS_ERR�жϷ��ص�ָ���Ƿ�Ϸ��� @@@driver@@@*/
	if(IS_ERR(my_dev->cls)){
		printk("Err, init class_create failed!\n");
		ret = -1;
		goto class_create_error;

	}

	/*�����豸*/
	my_dev->dev = device_create(my_dev->cls, NULL, my_dev->devno, NULL,"test");
	if(IS_ERR(my_dev->dev)){
		printk("Err, init device_create failed!\n ");
		ret = -1;
		goto device_create_error;
	}

	return 0;

device_create_error:
	class_destroy(my_dev->cls);
class_create_error:

	cdev_del(&my_dev->cdev);
cdev_add_error:

	unregister_chrdev_region(my_dev->devno,1);
register_chrdev_error:

	kfree(my_dev);


	return ret;
}



static void __exit test_exit(void)
{
	device_destroy(my_dev->cls, my_dev->devno);
	class_destroy(my_dev->cls);
	cdev_del(&my_dev->cdev);
	unregister_chrdev_region(my_dev->devno, 1);



}

module_init(test_init);  
module_exit(test_exit);  


MODULE_AUTHOR("yshisx");  
MODULE_LICENSE("Dual BSD/GPL"); 





