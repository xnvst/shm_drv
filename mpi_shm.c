/* device driver: shm  */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/mpishm.h>
#include "mpi_shm.h"

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DBG(fmt...) printk(fmt)
#else
#define DBG(fmt...)
#endif

static dev_t shm; 
static struct class *dev_class; 

#define SHM_VERSION   4

#define DEV_NAME "mpishm"
#define CLASS_NAME "mpishm_dev"
#define NUMBER_SHM_DEVICE  8

#define SHM_CNT_MAX			2
#define QUERY_FREQ			10

#define SHM_STATE_WAIT 0
#define SHM_STATE_ACTIVE 1
struct Shm_data
{
	wait_queue_head_t m_wait_queue;
	//atomic_t m_state;  
	unsigned m_state;
	struct semaphore m_sem; 
	struct cdev m_cdev;
	struct Mpi_channel *m_channel;
	char *m_buffer;
	unsigned m_data_available;
	unsigned long time_stamp;
	struct list_head m_data_list;
};

static struct list_head m_data_list_head;
extern struct Mpi_mutex upgrade_lock;
extern struct Mpi_mutex sync_shm_lock;
static wait_queue_head_t sync_shm_wait;
static int sync_shm_condition = 0;
static int sync_shm_cnt = 0;
static struct semaphore global_sem;
static struct timer_list shm_timer;
static struct semaphore list_sem;

static struct Shm_data the_shm_data[NUMBER_SHM_DEVICE];

static unsigned share_memory_buffer_size[NUMBER_SHM_DEVICE] = 
{
   4096,4096,4096,4096,65535,4096,4096,65535
};

static char buffer1[2][4096];
static char buffer2[2][4096];
static char buffer3[2][4096];
static char buffer4[2][4096];
static char buffer5[2][65535];
static char buffer6[2][4096];
static char buffer7[2][4096];
static char buffer8[2][65535];

static char *share_memory_read_buffer[NUMBER_SHM_DEVICE] = 
{
   buffer1[0],buffer2[0],buffer3[0],buffer4[0],buffer5[0],buffer6[0],buffer7[0],buffer8[0],
};

static char *share_memory_write_buffer[NUMBER_SHM_DEVICE] = 
{
   buffer1[1],buffer2[1],buffer3[1],buffer4[1],buffer5[1],buffer6[1],buffer7[1],buffer8[1],
};

static int dev_open(struct inode *pinode, struct file *filp)
{
   struct Shm_data *pdata;

   pdata = container_of(pinode->i_cdev, struct Shm_data, m_cdev);
   filp->private_data = pdata;
   
   DBG(KERN_INFO "MPI Shm Driver: open(), major[%d],minor[%d],pinode[%p],filp[%p],pdata[%p]\n",
      MAJOR(pinode->i_rdev),MINOR(pinode->i_rdev),pinode,filp,pdata);
   return 0;
}

static int dev_close(struct inode *pinode, struct file *filp)
{
   struct Shm_data *pdata =container_of(pinode->i_cdev, struct Shm_data, m_cdev);
   DBG(KERN_INFO "MPI Shm Driver: close(),pinode[%p], filp[%p],pdata[%p]\n",pinode,filp,pdata);
   return 0;
}

static ssize_t dev_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
   struct Shm_data *pdata = filp->private_data;
   unsigned data_len = 0;
   ssize_t ret = -ENOMEM;
   
   DBG(KERN_INFO "MPI Shm Driver: read(), filp[%p],pdata[%p],buf[%p],len[0x%x]\n",filp,pdata,buf,len);
   
   if (down_interruptible(&list_sem))return -ERESTARTSYS;
   if (pdata->time_stamp == 0xFFFFFFFF){
		list_del(&(pdata->m_data_list));
   }
   sync_shm_cnt--;
   up(&list_sem);   
   
   if (down_interruptible(&pdata->m_sem))return -ERESTARTSYS;

   //atomic_set(&pdata->m_state, SHM_STATE_WAIT);
   pdata->m_state = SHM_STATE_WAIT;
   if((pdata->m_data_available)&&pdata->m_channel)
   {
      data_len = (pdata->m_data_available > len)? len:pdata->m_data_available;
      ret = copy_to_user(buf,pdata->m_channel->m_buffer,data_len);
      if(ret)
      {
         printk("Failed to copy data to user, ret[0x%x]\n",ret);
         data_len = 0;
         goto read_out;
      }
      else
      {
         pdata->m_data_available = 0;
      }
   }
   else
   {
      DBG("No data available for read [%d]\n", pdata->m_channel->m_channel);
   }  

read_out:
   
   up (&pdata->m_sem);

   wake_up_interruptible(&pdata->m_wait_queue);   

   return data_len;
}

static ssize_t dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
   struct Shm_data *pdata = filp->private_data;
   struct Mpi_channel *channel = 0;
   ssize_t ret = -ENOMEM;
   unsigned write_len = 0;
   struct list_head *ptr;
   
   DBG(KERN_INFO "MPI Shm Driver: write(),filp[%p],pdata[%p],buf[%p],len[0x%x]\n",filp,pdata,buf,len);
   
   if(len>0)
   {	
      channel = pdata->m_channel;

	  if (down_interruptible(&list_sem)) return -ERESTARTSYS;     
	  pdata->time_stamp = jiffies;	  
	  list_add_tail(&(pdata->m_data_list), &m_data_list_head);
	  sync_shm_cnt++;
	  up(&list_sem);	
	  
      if(down_interruptible(&pdata->m_sem))return -ERESTARTSYS;	  
	  
      if(channel)
      {
         write_len = (channel->m_send_length>len)?len:channel->m_send_length;
         ret=copy_from_user((void *)pdata->m_buffer,buf,write_len);
         if(ret)
         {
            write_len = 0;
            printk("Failed to copy data from user\n");
            goto out;
         }
		 
		 if (down_interruptible(&global_sem)){
			return -ERESTARTSYS;
		 }	
         mpi_channel_send(channel, (void *)pdata->m_buffer, write_len);
         ++write_len;
		 ret=sync_shm_lock_func(1);
         if(ret<=0)
         {
            write_len = 0;
			printk(KERN_INFO "Failed to sync shm lock: %d\n",ret);
			if (down_interruptible(&list_sem))return -ERESTARTSYS;
			list_del(&(pdata->m_data_list));

   			sync_shm_cnt--;
   			up(&list_sem);
         }		 
		 up(&global_sem);	
      }

out:
      up (&pdata->m_sem);	  
   }
   return write_len;
}

static int wakeup_shm_device(struct Shm_data *pdata)
{
   DBG("To wake up shm device,pdata[%p]\n",pdata);
   BUG_ON(!pdata);
   
   pdata->m_state = SHM_STATE_ACTIVE;
   
   wake_up_interruptible(&pdata->m_wait_queue);   
   return 0;
}


static int dev_ioctl(struct inode *pinode, struct file *filp, unsigned int cmd, unsigned long arg)
{
   struct Shm_data *pdata = filp->private_data;
   DBG("<1>dev ioctl, cmd: 0x%x, arg: %lx. pinode[%p], filp[%p],pdata[%p]\n", cmd, arg,pinode,filp, pdata);

   switch (cmd) 
   {
      case SHM_SET_MODE:
         DBG("ioctl: [%d], set mode[%ld]\n", cmd, *(unsigned long *)arg);
         wakeup_shm_device(pdata);
         break;
      case SHM_SERVICE:
         DBG("ioctl: [%d], service[%ld]\n", cmd, *(unsigned long *)arg);
         break;
      case SHM_UNLOCK_CORE0:
		 Mpi_unlock(&upgrade_lock);
         break;
      case SHM_LOCK_SYNC:
         break;
      case SHM_UNLOCK_SYNC:
         break;
      case SHM_UNLOCK_SYNC_NEED:
		 sync_shm_lock_func(0);
         break;			 
      default:
         break;
   }

   return 0;
}

static unsigned int dev_poll(struct file *filp, poll_table *wait)
{
   struct Shm_data *pdata = filp->private_data;
   unsigned int mask = 0;

   down(&pdata->m_sem);
   poll_wait(filp, &pdata->m_wait_queue,  wait);
   //if (atomic_read(&pdata->m_state) == SHM_STATE_ACTIVE)
   if(pdata->m_state == SHM_STATE_ACTIVE)
   {
      mask |= POLLIN | POLLRDNORM;
   }      
   up(&pdata->m_sem);
   return mask;
}

static int mpi_shm_callback_func(void *data)
{
   int ret = 0;
   struct Mpi_channel *channel = (struct Mpi_channel *)data;
   char *buffer;
   
   struct Shm_data *pdata = (struct Shm_data *)channel->m_private_data;
   buffer = channel->m_buffer;
   
   DBG("%s: channel[%p],number[%d]\n", __FUNCTION__, data,channel->m_channel);
   
   mb();
   ret = mpi_shm_get(channel->m_recv_handler,(void *)buffer, channel->m_recv_length, 0);
   pdata->m_data_available = ret;

   DBG("channel[%p, %d],%d ms\n",data,channel->m_channel,(jiffies-pdata->time_stamp)*1000/HZ);

   pdata->time_stamp = 0xFFFFFFFF;
 
   wakeup_shm_device(pdata);

   sync_shm_lock_func(0);
   
   return ret;      
}
 


static struct file_operations shm_fops =
{
  .owner = THIS_MODULE,
  .open = dev_open,
  .release = dev_close,
  .read = dev_read,
  .write = dev_write,
  .ioctl = dev_ioctl,
  .poll = dev_poll,
};

int shm_setup_dev(struct Shm_data *dev_data, int index)
{
   int ret = 0;
   int devno = MKDEV(MAJOR(shm), MINOR(shm) + index);
   BUG_ON(!dev_data);

   DBG("%s: pdata[%p], index[%d]\n", __FUNCTION__, dev_data, index);
   memset((void *)dev_data,0, sizeof(*dev_data));
   init_waitqueue_head(&(dev_data->m_wait_queue));
   init_MUTEX(&dev_data->m_sem);
   //atomic_set(&dev_data->m_state, SHM_STATE_WAIT);
   dev_data->m_state = SHM_STATE_WAIT;

   if (device_create(dev_class, NULL, devno, "mpishm%d",index) == NULL)
   {
      printk(KERN_INFO "Driver init: failed creating device[%d]\n",index);
      return -1;
   }

   cdev_init(&dev_data->m_cdev, &shm_fops);
   dev_data->m_cdev.owner = THIS_MODULE;
   if (cdev_add(&dev_data->m_cdev, devno, 1) == -1)
   {
      device_destroy(dev_class, devno);
      return -1;
   }
   dev_data->m_channel = mpi_create_channel_with_data(index+1, share_memory_buffer_size[index],mpi_shm_callback_func,
                                                                     (void *)dev_data, share_memory_read_buffer[index]);

   if(!dev_data->m_channel)
   {
      device_destroy(dev_class, devno);
      return -1;
   }

   dev_data->m_buffer = share_memory_write_buffer[index];
   dev_data->m_data_available = 0;
   dev_data->time_stamp = 0;
   
   return ret;
}

static void shm_dev_cleanup(unsigned index)
{
   int i;
   for(i=0;i<index;++i)
   {
      cdev_del(&the_shm_data[i].m_cdev);
      device_destroy(dev_class, MKDEV(MAJOR(shm), MINOR(shm) + i));
      mpi_destroy_channel(the_shm_data[i].m_channel);
   }
   class_destroy(dev_class);
   unregister_chrdev_region(shm, NUMBER_SHM_DEVICE);     
}

static void query_shm_list(unsigned long data){
	struct list_head *ptr;
	struct list_head *next;
	struct Shm_data *shm_ptr;
	DBG("%s: %d\n",__FUNCTION__,sync_shm_cnt);
	if (sync_shm_cnt>=SHM_CNT_MAX){ 	
		printk("Warning: The number of entries in the list is [%d]\n",sync_shm_cnt);
		if (down_interruptible(&list_sem))return -ERESTARTSYS;
		list_for_each_safe(ptr, next, &m_data_list_head){
			shm_ptr = list_entry(ptr, struct Shm_data, m_data_list);
			printk("[%p,%d,0x%x,0x%x]\n",shm_ptr,shm_ptr->m_channel->m_channel,shm_ptr->time_stamp,jiffies);
		}
		up(&list_sem);		
	}	
	mod_timer(&shm_timer, jiffies + QUERY_FREQ*HZ);	
}	

static int __init shm_init(void) /* Constructor */
{
   int i = 0; //, j = 0;
   int err = 0;
   int ret;
   
   DBG(KERN_INFO "Driver init: MPI shm registered\n");
   
   if (alloc_chrdev_region(&shm, 0, NUMBER_SHM_DEVICE, DEV_NAME) < 0)
   {
      printk(KERN_INFO "Driver init: failed allocating devices\n");
      return -1;
   }
   
   if ((dev_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL)
   {
      printk(KERN_INFO "Driver init: failed creating class\n");
      unregister_chrdev_region(shm, NUMBER_SHM_DEVICE);
      return -1;
   }

   for(i=0;i<NUMBER_SHM_DEVICE;++i)
   {
      err = shm_setup_dev(&the_shm_data[i],i);
      if(err==-1)
      {
         shm_dev_cleanup(i);
         return -1;                          
      }            
   }   
      
   ret = Mpi_mutex_init(&upgrade_lock,MPI_SYSTEM_LOCK_CORE0);
   if(ret!=0)
   {
      printk("can not init memory lock\n");
   }

   ret = Mpi_mutex_init(&sync_shm_lock,MPI_SYSTEM_LOCK_SYNC_SHM);
   if(ret!=0)
   {
      printk("can not init memory lock\n");
   }   

   init_waitqueue_head(&sync_shm_wait);
   
   init_MUTEX(&global_sem);
   
   init_MUTEX(&list_sem);
   INIT_LIST_HEAD(&m_data_list_head);   
   
   init_timer(&shm_timer);
   shm_timer.data = 0;
   shm_timer.function = query_shm_list;
   shm_timer.expires = jiffies + QUERY_FREQ*HZ;
   add_timer(&shm_timer);
   
   DBG("<1>Inserting shm module, version[%d]\n",SHM_VERSION);
   return 0;
}
 
static void __exit shm_exit(void) /* Destructor */
{

   shm_dev_cleanup(NUMBER_SHM_DEVICE);
   del_timer(&shm_timer);	
   DBG(KERN_INFO "Driver exit: shm unregistered\n");
}

int sync_shm_lock_func(int lock)
{
	int ret = 0;
	sync_shm_condition = 0;
	if (lock){
		ret = wait_event_interruptible_timeout(sync_shm_wait, sync_shm_condition!=0, 10*HZ);
		sync_shm_condition = 0;	
	}
	else{
		sync_shm_condition = 1;	
		wake_up_interruptible(&sync_shm_wait);
	}
	return ret;
}
EXPORT_SYMBOL(sync_shm_lock_func);



module_init(shm_init);
module_exit(shm_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fcao");
MODULE_DESCRIPTION("Linux Character Device Driver");

