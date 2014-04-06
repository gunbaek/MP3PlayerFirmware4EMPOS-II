//fimename : pushbuttonport.c

#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/signal.h>

#define PUSHBUTTONPORT_MAJOR   0
#define PUSHBUTTONPORT_NAME    "PUSHBUTTON_PORT"
#define PUSHBUTTONPORT_MODULE_VERSION "PUSHBUTTON V0.1"
#define PUSHBUTTONPORT_ADDRESS 0xf1500000
#define PUSHBUTTONPORT_ADDRESS_RANGE 1
#define TIMER_INTERVAL			20

//Global variable
static int pushbuttonport_usage = 0;
static int pushbuttonport_major = 0;
static struct timer_list mytimer;
static unsigned char value;
static unsigned char *add;
static pid_t id;

// define functions...
int pushbuttonport_open(struct inode *minode, struct file *mfile) {
	if(pushbuttonport_usage != 0) return -EBUSY;

	MOD_INC_USE_COUNT;
	pushbuttonport_usage = 1;
	add = (unsigned char *)PUSHBUTTONPORT_ADDRESS;
	return 0;
}

int pushbuttonport_release(struct inode *minode, struct file *mfile) {
	MOD_DEC_USE_COUNT;
	pushbuttonport_usage = 0;
	del_timer(&mytimer);
	return 0;
}

//ADD your functions

void mypollingfunction(unsigned long data){
	value = ~(*add);
	if(value != 0x00) kill_proc(id,SIGUSR1,1);
	mytimer.expires = jiffies + TIMER_INTERVAL;
	add_timer(&mytimer);
}

ssize_t pushbuttonport_write(struct file *inode, const char * gdata, size_t length, loff_t * off_what){
	get_user(id,(int *)gdata);
	init_timer(&mytimer);
	mytimer.expires = jiffies + TIMER_INTERVAL;
	mytimer.function = &mypollingfunction;
	add_timer(&mytimer);
	return length;
}

ssize_t pushbuttonport_read(struct file *inode, char *gdata, size_t length, loff_t *off_what){
	copy_to_user(gdata,&value,1);
	return length;
}

struct file_operations pushbuttonport_fops = {
	read: pushbuttonport_read,
    write: pushbuttonport_write,
	open: pushbuttonport_open,
	release: pushbuttonport_release,
};

int init_module(void) {
	int result;
	result = register_chrdev(PUSHBUTTONPORT_MAJOR,PUSHBUTTONPORT_NAME,&pushbuttonport_fops);

	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	pushbuttonport_major = result;
	if(!check_region(PUSHBUTTONPORT_ADDRESS,PUSHBUTTONPORT_ADDRESS_RANGE))
		request_region(PUSHBUTTONPORT_ADDRESS,PUSHBUTTONPORT_ADDRESS_RANGE,PUSHBUTTONPORT_NAME);
	else printk("driver : unable to register this!\n");
	printk("init module, pushbuttonport major number : %d\n",result);

	return 0;
}

void cleanup_module(void) {
	release_region(PUSHBUTTONPORT_ADDRESS,PUSHBUTTONPORT_ADDRESS_RANGE);
	if(unregister_chrdev(pushbuttonport_major,PUSHBUTTONPORT_NAME))
		printk("driver : %s DRIVER CLEANUP FALLED\n",PUSHBUTTONPORT_NAME);
}
