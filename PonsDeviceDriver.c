#include <linux/init.h>      // Macros used to mark up functions e.g., __init __exit
#include <linux/module.h>    // Core header for loading LKMs into the kernel
#include <linux/kernel.h>    // Contains types, macros, functions for the kernel - printk()
#include <linux/errno.h>     // error codes
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/types.h>  
#include <linux/kdev_t.h> 
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/pfn.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <"McodeMod.h">

#define LEDON 250000
#define DASHON 750000

#define  DEVICE_NAME "morsecode"    ///< The device will appear at /dev/morsecode using this value
#define  CLASS_NAME  "morse"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Victor Gonzalez");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static DEFINE_MUTEX(morsecode_mutex); 

static int    majorNumber;                  ///< Store the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  morsecodeClass  = NULL; ///< The device-driver class struct pointer
static struct device* morsecodeDevice = NULL; ///< The device-driver device struct pointer       

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);


static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

void BBBremoveTrigger(void);
void BBBstartHeartbeat(void);
void BBBledOn(void);
void BBBledOff(void);


#define GPIO1_START_ADDR 0x4804C000
#define GPIO1_END_ADDR   0x4804e000
#define GPIO1_SIZE (GPIO1_END_ADDR - GPIO1_START_ADDR)

#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190
#define USR3 (1<<24)
#define USR0 (1<<21)

#define USR_LED USR0
#define LED0_PATH "/sys/class/leds/beaglebone:green:usr0"

static volatile void *gpio_addr;
static volatile unsigned int *gpio_setdataout_addr;
static volatile unsigned int *gpio_cleardataout_addr;


ssize_t write_vaddr_disk(void *, size_t);
int setup_disk(void);
void cleanup_disk(void);
static void disable_dio(void);

static struct file * f = NULL;
static int reopen = 0;
static char *filepath = 0;
static char fullFileName[1024];
static int dio = 0;

static char *name = "world";   ///< An example LKM argument -- default value is "world"

module_param(name, charp, S_IRUGO);
///< Param desc. charp = char ptr, S_IRUGO can be read/not changed

MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter descript


static int __init hello_init(void){
      printk(KERN_INFO "morsecode: Initializing the morsecode LKM\n");

   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "morsecode failed to register a major number\n");
      return majorNumber;
   }
   else printk(KERN_INFO "morsecode: registered correctly with major number %d\n", majorNumber);

   morsecodeClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(morsecodeClass)){           // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(morsecodeClass);     // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "morsecode: device class registered correctly\n");

   morsecodeDevice = device_create(morsecodeClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(morsecodeDevice)){          // Clean up if there is an error
      class_destroy(morsecodeClass);      // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(morsecodeDevice);
   }
   else printk(KERN_INFO "morsecode: device class created correctly\n"); // Made it! device was initialized
   
   mutex_init(&morsecode_mutex);          // Initialize the mutex dynamically
   
   return 0;
}

   printk(KERN_INFO "TDR: Hello %s from morse LKM!\n", name);
   gpio_addr = ioremap(GPIO1_START_ADDR, GPIO1_SIZE);
   if(!gpio_addr) {
     printk (KERN_ERR "HI: ERROR: Failed to remap memory for GPIO Bank 1.\n");
   }

   gpio_setdataout_addr   = gpio_addr + GPIO_SETDATAOUT;
   gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

   BBBremoveTrigger(); //OLD
   BBBledOn();
   msleep(1000);
   BBBledOff();
   msleep(1000);
   BBBledOn();

   return 0;
}



static void __exit hello_exit(void){
   mutex_destroy(&morsecode_mutex);                       
   device_destroy(morsecodeClass, MKDEV(majorNumber, 0)); // remove the device
   class_unregister(morsecodeClass);                      // unregister the device class
   class_destroy(morsecodeClass);                         // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);         // unregister the major number
   printk(KERN_INFO "morsecode: Goodbye handsome devil\n");

   BBBledOff(); //OLD
   BBBstartHeartbeat();
   printk(KERN_INFO "TDR: Goodbye %s from the morse LKM!\n", name);
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
/*   int error_count = 0;
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){           
      printk(KERN_INFO "morsecode: Sent %d characters to the handsome devil\n", size_of_message);
      return (size_of_message=0); // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "morsecode: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;      // Failed -- return a bad address message (i.e. -14)
   }
**/

	int index=0;
	
	while (buffer[index] != '\0') {
		
		char * letter;
		int i=0;
		
		letter = mcodestring(buffer[index]);

		while (letter[i] != '\0'){
			
		
			/*turn on LED*/
			switch (letter[i])
			{
			case '.':
				BBBledOn();
				msleep(250);
			break;
			case '-':
				BBBledOn();
				msleep(750);
			break;
			case ' ':
				BBBledOff();
				msleep(1750);
			break;
			}

			/*Inter ./- Spacing*/
			BBBledOff();
   			msleep(250);
			i++;
		}
			/*Inter-char Spacing*/
			BBBledOff();
   			msleep(750);
		index++;
		
	}
	
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){

   sprintf(message, "%s(%lu letters)", buffer, len);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "morsecode: Received %lu characters from the handsome devil\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&morsecode_mutex);                      // release the mutex (i.e., lock goes up)
   printk(KERN_INFO "morsecode: Device successfully closed by the handome devils handsome PC\n");
   return 0;
}

static int dev_open(struct inode *inodep, struct file *filep){

   if(!mutex_trylock(&morsecode_mutex)){                  
    printk(KERN_ALERT "morsecode: Device in use by another process");
    return -EBUSY;
   }
   numberOpens++;
   printk(KERN_INFO "morsecode: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

module_init(hello_init);
module_exit(hello_exit);

//OLD
void BBBremoveTrigger(){
   // remove the trigger from the LED
   int err = 0;
  
  strcpy(fullFileName, LED0_PATH);
  strcat(fullFileName, "/");
  strcat(fullFileName, "trigger");
  printk(KERN_INFO "File to Open: %s\n", fullFileName);
  filepath = fullFileName; // set for disk write code
  err = setup_disk();
  err = write_vaddr_disk("none", 4);
  cleanup_disk();
}

void BBBstartHeartbeat(){
   // start heartbeat from the LED
     int err = 0;
  

  strcpy(fullFileName, LED0_PATH);
  strcat(fullFileName, "/");
  strcat(fullFileName, "trigger");
  printk(KERN_INFO "File to Open: %s\n", fullFileName);
  filepath = fullFileName; // set for disk write code
  err = setup_disk();
  err = write_vaddr_disk("heartbeat", 9);
  cleanup_disk();
}

void BBBledOn(){
*gpio_setdataout_addr = USR_LED;
}


void BBBledOff(){
*gpio_cleardataout_addr = USR_LED;
}


static void disable_dio() {
   dio = 0;
   reopen = 1;
   cleanup_disk();
   setup_disk();
}

int setup_disk() {
   mm_segment_t fs;
   int err;

   fs = get_fs();
   set_fs(KERNEL_DS);
	
   if (dio && reopen) {	
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_SYNC | O_DIRECT, 0444);
   } else if (dio) {
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC | O_SYNC | O_DIRECT, 0444);
   }
	
   if(!dio || (f == ERR_PTR(-EINVAL))) {
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, 0444);
      dio = 0;
   }
   if (!f || IS_ERR(f)) {
      set_fs(fs);
      err = (f) ? PTR_ERR(f) : -EIO;
      f = NULL;
      return err;
   }

   set_fs(fs);
   return 0;
}

void cleanup_disk() {
   mm_segment_t fs;

   fs = get_fs();
   set_fs(KERNEL_DS);
   if(f) filp_close(f, NULL);
   set_fs(fs);
}

ssize_t write_vaddr_disk(void * v, size_t is) {
   mm_segment_t fs;

   ssize_t s;
   loff_t pos;

   fs = get_fs();
   set_fs(KERNEL_DS);
	
   pos = f->f_pos;
   s = vfs_write(f, v, is, &pos);
   if (s == is) {
      f->f_pos = pos;
   }					
   set_fs(fs);
   if (s != is && dio) {
      disable_dio();
      f->f_pos = pos;
      return write_vaddr_disk(v, is);
   }
   return s;
}
