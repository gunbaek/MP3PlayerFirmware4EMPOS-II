//fimename : segmentport.c

#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include <linux/ioport.h>

#define SEGMENTPORT_MAJOR   0
#define SEGMENTPORT_NAME    "SEGMENT PORT"
#define SEGMENTPORT_MODULE_VERSION "SEGMENT PORT V0.1"
#define SEGMENTPORT_ADDRESS_LOW 0xf1300000
#define SEGMENTPORT_ADDRESS_HIGH 0xf1400000
#define SEGMENTPORT_ADDRESS_RANGE 4

#define SEGMENTPORT_BASE				0xbb
#define SEGMENT_WRITE_LOW		 _IOW(SEGMENTPORT_BASE,0,4)
#define SEGMENT_WRITE_HIGH	     _IOW(SEGMENTPORT_BASE,1,4)
#define SEGMENT_TIMER_START      _IOW(SEGMENTPORT_BASE,2,4)
#define SEGMENT_TIMER_STOP	     _IOW(SEGMENTPORT_BASE,3,4)
#define SEGMENT_TIMER_INIT       _IOW(SEGMENTPORT_BASE,4,4)
#define SEGMENT_TIMER_OFF	     _IOW(SEGMENTPORT_BASE,5,4)

#define TIMER_INTERVAL			100

unsigned int *addr_low;
unsigned int *addr_high;

//Global variable
static struct timer_list mySegTimer;

static int segmentport_usage = 0;
static int segmentport_major = 0;
static int timer_control = 0;
// timer on if control is 1
// timer on if control is 0

static int sec;
static int min;
//time variable 

static void segTimer(){
	if( (sec+1) == 60 ){ 
		min = (++min)%60;
		segment_write_high(min);
	}
	sec = (++sec)%60;
	segment_write_low(sec);

	//restart if timer_control is 1
	if(timer_control == 1){
		mySegTimer.expires = jiffies + TIMER_INTERVAL;
		add_timer(&mySegTimer);
	}
}

// define functions...
int segmentport_open(struct inode *minode, struct file *mfile) {
	if(segmentport_usage != 0) return -EBUSY;
	MOD_INC_USE_COUNT;
	segmentport_usage = 1;
	
	addr_low = (unsigned int *)(SEGMENTPORT_ADDRESS_LOW);
	addr_high = (unsigned int *)(SEGMENTPORT_ADDRESS_HIGH);
	
	return 0;
}

int segmentport_release(struct inode *minode, struct file *mfile) {
	MOD_DEC_USE_COUNT;
	segmentport_usage = 0;
	timer_control = 0;
	del_timer(&mySegTimer);
	return 0;
}

unsigned int Getsegcode(int x)
{
	unsigned int code;
	switch (x) {
		case 0x0 : code = 0x3f; break;
		case 0x1 : code = 0x06; break;
		case 0x2 : code = 0x5b; break;
		case 0x3 : code = 0x4f; break;
		case 0x4 : code = 0x66; break;
		case 0x5 : code = 0x6d; break;
		case 0x6 : code = 0x7d; break;
		case 0x7 : code = 0x07; break;
		case 0x8 : code = 0x7f; break;
		case 0x9 : code = 0x6f; break;
		case 0xA : code = 0x77; break;
		case 0xB : code = 0x7c; break;
		case 0xC : code = 0x39; break;
		case 0xD : code = 0x5e; break;
		case 0xE : code = 0x79; break;
		case 0xF : code = 0x71; break;
		default  : code = 0;    break;
	}
	return code;
}


ssize_t segmentport_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	unsigned short val;

	return length;
}
void segment_write_low(unsigned int val){
		int val_low, val_high;

		val_low = val % 10;
		val_high = val / 10;
		
		val_low = Getsegcode(val_low);
		val_high = Getsegcode(val_high);

		*addr_low = (val_high << 8)|val_low;
}
void segment_write_high(unsigned int val){
		int val_low, val_high;

		val_low = val % 10;
		val_high = val / 10;
		
		val_low = Getsegcode(val_low);
		val_high = Getsegcode(val_high);
		*addr_high = (val_high << 8)|(val_low+0x80);

}
int segmentport_ioctl(struct inode *inode, struct file *file,unsigned int cmd,unsigned long gdata) {
	unsigned int val;
	unsigned int val_low,val_high;
	unsigned int hex_val;
	
	switch(cmd){
	case SEGMENT_WRITE_LOW:
		get_user(val,(unsigned int *)gdata);
		segment_write_low(val);
		break;
	case SEGMENT_WRITE_HIGH:
		get_user(val,(unsigned int *)gdata);
		segment_write_high(val);
		break;
	case SEGMENT_TIMER_START :
		timer_control = 1;
		init_timer(&mySegTimer);
		mySegTimer.expires = jiffies + TIMER_INTERVAL;
		mySegTimer.function = &segTimer;
		add_timer(&mySegTimer);
		break;
	case SEGMENT_TIMER_STOP :
		timer_control = 0;
		del_timer(&mySegTimer);
		break;
	case SEGMENT_TIMER_INIT :
		sec = 0;
		min = 0;
		val_low = Getsegcode(0);
		val_high = Getsegcode(0);
		*addr_low = (val_high << 8)|val_low;
		*addr_high = (val_high << 8)| (val_low+0x80);
		break;
	case SEGMENT_TIMER_OFF :
		sec = 0;
		min = 0;
		val_low = Getsegcode(0);
		val_high = Getsegcode(0);
		*addr_low = 0;
		*addr_high = 0;
		break;

	default:
	//	printk("driver : no such command!\n");
		return -ENOTTY;
	}

	return 0;
}

struct file_operations segment_fops = {
	write: segmentport_write,
	ioctl: segmentport_ioctl,
	open: segmentport_open,
	release: segmentport_release,
};

int init_module(void) {

	int result;
	result = register_chrdev(SEGMENTPORT_MAJOR,SEGMENTPORT_NAME,&segment_fops);
	if(result<0)
	{
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	segmentport_major=result;

	
	if(!check_region(SEGMENTPORT_ADDRESS_LOW,SEGMENTPORT_ADDRESS_RANGE)||
			!check_region(SEGMENTPORT_ADDRESS_HIGH,SEGMENTPORT_ADDRESS_RANGE)) 
	{
			request_region(SEGMENTPORT_ADDRESS_LOW,SEGMENTPORT_ADDRESS_RANGE,SEGMENTPORT_NAME);
			request_region(SEGMENTPORT_ADDRESS_HIGH,SEGMENTPORT_ADDRESS_RANGE,SEGMENTPORT_NAME);
			printk("init module, segmentport major number : %d\n",result);
	}
	else printk("driver : unable to register this!\n");
	
	return 0;
}

void cleanup_module(void) {
	release_region(SEGMENTPORT_ADDRESS_LOW,SEGMENTPORT_ADDRESS_RANGE);
	release_region(SEGMENTPORT_ADDRESS_HIGH,SEGMENTPORT_ADDRESS_RANGE);	

	if(unregister_chrdev(segmentport_major,SEGMENTPORT_NAME))
		printk("driver : %s DRIVER CLEANUP Ok\n", SEGMENTPORT_NAME);
	else
		printk("driver : %s DRIVER CLEANUP Failed\n", SEGMENTPORT_NAME);

}
