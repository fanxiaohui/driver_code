/*		
 *	name:	kernel_lock_test.c
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




#define GLOBALMEM_SIZE	0x1000	/*ȫ���ڴ����4K�ֽ�*/
#define MEM_CLEAR 0x1  /*��0ȫ���ڴ�*/

#define GLOBALMEM_MAJOR 235    /*Ԥ���globalmem�����豸��*/
static int globalmem_major = GLOBALMEM_MAJOR;
/*globalmem�豸�ṹ��*/
struct globalmem_dev
{
	dev_t devno;
	struct cdev cdev; /*cdev�ṹ��*/
	struct device *dev;
	struct class *class;
	unsigned char mem[GLOBALMEM_SIZE]; /*ȫ���ڴ�*/    
	rwlock_t lock;
};



struct globalmem_dev *globalmem_devp; /*�豸�ṹ��ָ��*/


/* seek�ļ���λ���� */
static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{  
	loff_t ret = 0;    
	switch (orig)  
	{    
		case 0:   /*����ļ���ʼλ��ƫ��*/      
			if (offset < 0)      
			{        
				ret =  - EINVAL;        
				break;      
			}      
			if ((unsigned int)offset > GLOBALMEM_SIZE)     
			{        
				ret =  - EINVAL;       
				break;     
			}      
			filp->f_pos = (unsigned int)offset;     
			ret = filp->f_pos;     
		break;    
		case 1:   /*����ļ���ǰλ��ƫ��*/      
			if ((filp->f_pos + offset) > GLOBALMEM_SIZE)   
			{
				ret =  - EINVAL;       
				break;   
			}    
			if ((filp->f_pos + offset) < 0)      
			{      
				ret =  - EINVAL;       
				break;     
			}      
			filp->f_pos += offset;      
			ret = filp->f_pos;     
		break; 
		default:     
			ret =  - EINVAL;      
			break;  
		}  
	return ret;
}




/*������*/
static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size,  loff_t *ppos)
{  
	unsigned long p =  *ppos;  
	unsigned int count = size;  
	int ret = 0; 
	
	struct globalmem_dev *dev = filp->private_data; /*����豸�ṹ��ָ��*/  
	/*�����ͻ�ȡ��Ч��д����*/ 
	if (p >= GLOBALMEM_SIZE)   
		return count ?  - ENXIO: 0;  //ENXIO  :no such devices or address  
	if (count > GLOBALMEM_SIZE - p)    
		count = GLOBALMEM_SIZE - p;  
	read_lock(&(globalmem_devp->lock));			//������ 
	printk(KERN_EMERG "read lock!  current->pid=%d\n",current->pid);  /*�ں˿ռ�->�û��ռ�*/	  

	if (copy_to_user(buf, (void*)(dev->mem + p), count))	     
		ret =  - EFAULT; /* Bad address */	  
	else{	    
		*ppos += count;	    
		ret = count;	    	   
		printk(KERN_INFO "read %d bytes(s) from %ld\n", count, p);	
	}	
	//mdelay(5000);  æ�ȴ�����ʱ�����ã�������	  
	// ssleep(5);//  ���˶�д������sleep����Ȼ���ó�CPU�����������л�������������������2���������� 
	read_unlock(&(globalmem_devp->lock));			//������
	printk(KERN_EMERG "read unlock!  current->pid=%d\n",current->pid); 
	return ret;
}

//10 byte 20
/*д����*/
static ssize_t globalmem_write(struct file *filp, const char __user *buf,
  size_t size, loff_t *ppos)
{
  unsigned long p =  *ppos;
  unsigned int count = size;
  int ret = 0;

  struct globalmem_dev *dev = filp->private_data; /*����豸�ṹ��ָ��*/
  
  /*�����ͻ�ȡ��Ч��д����*/
  if (p >= GLOBALMEM_SIZE)
    return count ?  - ENXIO: 0;
  if (count > GLOBALMEM_SIZE - p)
    count = GLOBALMEM_SIZE - p;
  write_lock(&(globalmem_devp->lock));	/*д����*/
  printk(KERN_EMERG "write lock!current->pid=%d \n",current->pid);
  /*�û��ռ�->�ں˿ռ�*/

	 if (copy_from_user(dev->mem + p, buf, count))
	    ret =  - EFAULT;
	  else
	  {
	    *ppos += count;
	    ret = count;
	    
	    printk(KERN_INFO "written %d bytes(s) from %ld\n", count, p);
	  }

  //ssleep(5);
  write_unlock(&(globalmem_devp->lock));	/*д����*/
  printk(KERN_EMERG "write unlock! current->pid=%d\n",current->pid);
  return ret;
}


/* ioctl�豸���ƺ��� */
static long globalmem_ioctl(struct file *filp, unsigned
  int cmd, unsigned long arg)
{
  struct globalmem_dev *dev = filp->private_data;/*����豸�ṹ��ָ��*/

  switch (cmd)
  {
    case MEM_CLEAR:
	//spin_lock(&(globalmem_devp->lock));	/*��ȡ������*/
	write_lock(&(globalmem_devp->lock));	
      memset(dev->mem, 0, GLOBALMEM_SIZE);   
	//spin_unlock(&(globalmem_devp->lock));	/*����*/
	write_unlock(&(globalmem_devp->lock));	
      printk(KERN_INFO "globalmem is set to zero\n");
	  
      break;

    default:
      return  - EINVAL;/* Invalid argument */

  }
  return 0;
}

int globalmem_open(struct inode *inode, struct file *filp)
{
  /*���豸�ṹ��ָ�븳ֵ���ļ�˽������ָ��*/
  filp->private_data = globalmem_devp;
  return 0;
}
/*�ļ��ͷź���*/
int globalmem_release(struct inode *inode, struct file *filp)
{
  return 0;
}


static const struct file_operations globalmem_fops =
{  
	.owner 		= THIS_MODULE,  
	.llseek 	= globalmem_llseek,  
	.read 		= globalmem_read,  
	.write 		= globalmem_write,  
	.open 		= globalmem_open,  
	.release 	= globalmem_release,
	.unlocked_ioctl = globalmem_ioctl,  
};


static int globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
	int err, devno = MKDEV(globalmem_major, index);
	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;   
	err = cdev_add(&dev->cdev, devno, 1);  
	if(err)    
		printk(KERN_NOTICE "Error %d adding LED%d", err, index);
	return err;
}




static int __init lock_test_init(void)
{

	int result;	
	int ret;
	
	globalmem_devp = kmalloc(sizeof(struct globalmem_dev), GFP_KERNEL);//GFP_KERNEL����ζ�ŷ�������������ں˿ռ�Ľ��̽��е�	
	if (!globalmem_devp)	/*����ʧ��*/	
	{	
		result =  - ENOMEM; 
		return -1;	
	}	
	globalmem_devp->devno = MKDEV(globalmem_major, 0);
	if(globalmem_major)	
		result = register_chrdev_region(globalmem_devp->devno, 1, "globalmem"); 
	else{	
		result = alloc_chrdev_region(&globalmem_devp->devno, 0, 1, "globalmem");	
		globalmem_major = MAJOR(globalmem_devp->devno); 
	}	
	if(result < 0)
		goto dev_num_fail;
	printk(KERN_WARNING "globalmem get major: %d\n", globalmem_major);
	
	memset(globalmem_devp, 0, sizeof(struct globalmem_dev));

	ret = globalmem_setup_cdev(globalmem_devp, 0);
	if(ret != 0)
		goto cdev_fail;
	rwlock_init(&(globalmem_devp->lock)); /*��̬��ʼ����д��*/
	globalmem_devp->class = class_create(THIS_MODULE, "my_class");	
	if(IS_ERR(globalmem_devp->class)){
		printk("Err: failed in creating class.\n");		 
		//return -1; 	
		goto class_fail;
	}	
	/* register your own device in sysfs, and this will cause udev to create corresponding device node */	
	globalmem_devp->dev = device_create(globalmem_devp->class, NULL, MKDEV(globalmem_major, 0), NULL, "globalmem");	
	if(IS_ERR(globalmem_devp->dev)){
		printk("Err: failed in device create. \n");
		goto device_fail;
	}
	return 0;

device_fail:
	class_destroy(globalmem_devp->class);

class_fail:
	
	cdev_del(&globalmem_devp->cdev);

cdev_fail:

	
	unregister_chrdev_region(globalmem_devp->devno, 1);	

dev_num_fail: 

	kfree(globalmem_devp);
	
	return result;


}

static void __exit lock_test_exit(void)
{
	device_destroy(globalmem_devp->class, globalmem_devp->devno);
	class_destroy(globalmem_devp->class);
	cdev_del(&globalmem_devp->cdev);
	unregister_chrdev_region(globalmem_devp->devno, 1);
}






module_init(lock_test_init);  
module_exit(lock_test_exit);  


MODULE_AUTHOR("yshisx");  
MODULE_LICENSE("Dual BSD/GPL"); 




