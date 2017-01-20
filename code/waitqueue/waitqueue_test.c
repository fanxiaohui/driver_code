/*		
 *	name:	waitqueuqe_test.c
 *	auth:	wuw
 *	date:	2016��1��30�� 18:47:31
 *	
 *
 */


#include <linux/device.h>  
#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>  
#include <linux/string.h>  
#include <linux/sysfs.h>  
#include <linux/stat.h> 
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/delay.h>


struct my_dev_t
{
	dev_t devno;
	struct cdev cdev;
	struct class *cls;
	struct device *dev;
	unsigned long condition;
	wait_queue_head_t dev_wait;

};


struct my_dev_t *my_dev;
static int dev_major = 0;













#define SET_SLEEP 1
#define SET_WAKE_UP 2



static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(cmd)
	{
		case SET_SLEEP:
		{
			//printk("+++++++++++++\n");
			//while(1);
			//printk("-------------\n");
			/*�������������ʾ��������ʱ���趨��ʱ���ж�ã�0Ϊ��ʱ��>0Ϊʣ���ʱ��*/
			ret = wait_event_interruptible_timeout(my_dev->dev_wait, my_dev->condition, msecs_to_jiffies(20*1000));
			printk("wait_event_interruptible_timeout is back !ret = %d \n", ret);
			
			if(ret > 0){
				ret = my_dev->condition;
				my_dev->condition = 0;
				
			}				
			return ret;
		}
		break;
		case SET_WAKE_UP:
		{
			printk("send wake up signal\n");
			my_dev->condition = 1;
			wake_up_interruptible(&my_dev->dev_wait);
			
		}
		break;
		default:
			 return  - EINVAL;/* Invalid argument */

	}

	return 0;
}




int my_open(struct inode *inode, struct file *filp)
{
	filp->private_data = my_dev;
	return 0;

}

int my_release(struct inode *inode, struct file *filp)
{
	return 0;

}

static const struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.unlocked_ioctl = my_ioctl,
	
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
	my_dev->dev = device_create(my_dev->cls, NULL, my_dev->devno, NULL, "my_dev");
	if(IS_ERR(my_dev->dev)){
		printk("Err, init device_create failed!\n ");
		ret = -1;
		goto device_create_error;
	}

	init_waitqueue_head(&my_dev->dev_wait);





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





