#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/thread_info.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DEVICE_NAME "DriverMorse"
#define CLASS_NAME "Morse"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alejandro Dominguez");
MODULE_DESCRIPTION("A Linux Device Driver for Morse Code");
MODULE_VERSION("1");

static DEFINE_MUTEX(morse_mutex);

static int    majorNumber;
static char   message[256] = {0};
static short  size_of_message;
static int    numberOpens = 0;
static struct class*  testcharClass  = NULL;
static struct device* testcharDevice = NULL;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};


static int __init dev_init(void){
	printk(KERN_INFO "[DriverMorse]: Device has been called.");
	
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "[DriverMorse]: Failed to register a major number\n");
		return majorNumber;
	}
	else printk(KERN_INFO "[DriverMorse]: Registered correctly with major number %d\n", majorNumber);
	
	testcharClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(testcharClass)){                // Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "[DriverMorse]: Failed to register device class\n");
		return PTR_ERR(testcharClass);          // Correct way to return an error on a pointer
	}
	else printk(KERN_INFO "[DriverMorse]: Device class registered correctly\n");
	
	testcharDevice = device_create(testcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(testcharDevice)){               // Clean up if there is an error
		class_destroy(testcharClass);           // Repeated code but the alternative is goto statements
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "[DriverMorse]: Failed to create the device\n");
		return PTR_ERR(testcharDevice);
	}
	else printk(KERN_INFO "[DriverMorse]: Device class created correctly\n");

	mutex_init(&morse_mutex);

	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);
 
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "[DriverMorse]: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "[DriverMorse]: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   printk(KERN_INFO "[DriverMorse]: Received %lu characters from the user\n", len); // We use %lu because len is an long unsigned
      
   const int* LED_HANDLE = 0x854;//led offset address for usr0
   
   int i,j;
	for(i=0; i<100; i++){ //
		for(j=0; j<100; j++){
			if(!(buffer[i][j]==NULL)){
				int ascii = (int)buffer[i][j];

				if(ascii == 46){ // this is for the dot
					fwrite('1',sizeof(char),1,LED_HANDLE);
					msleep(500);
					fwrite('0',sizeof(char),1,LED_HANDLE
					msleep(500);
					 // timing for in between dots and dashes
				}
				else if(ascii == 45){ // this is the dash
					fwrite('1',sizeof(char),1,LED_HANDLE);
					msleep(1500); // timing for inbetween dots and dashes
					fwrite('0',sizeof(char),1,LED_HANDLE);
					msleep(500);
				}
			}
			else j = 100;
		}
		msleep(1500); // this is for in between letters

		if(mc[i]==NULL) i = 100;
	}  

   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&morse_mutex); 
   printk(KERN_INFO "[DriverMorse]: Device successfully closed\n");
   return 0;
}

static int dev_open(struct inode *inodep, struct file *filep){
   if(!mutex_trylock(&morse_mutex)){    /// Try to acquire the mutex (i.e., put the lock on/down)
                                          /// returns 1 if successful and 0 if there is contention
      printk(KERN_ALERT "[DriverMorse]: Device in use by another process");
      return -EBUSY;
   }
   numberOpens++;
   printk(KERN_INFO "[DriverMorse]: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

void __exit dev_exit(void){
	mutex_destroy(&morse_mutex);
	device_destroy(testcharClass, MKDEV(majorNumber, 0));
	class_unregister(testcharClass);                     
	class_destroy(testcharClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "[DeviceDriver]: I trusted you.\n");
}

module_init(dev_init);
module_exit(dev_exit);
