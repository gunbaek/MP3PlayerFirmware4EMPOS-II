//fimename : discretelcdport.c

#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/ioctl.h>
#include <linux/ioport.h>

#define DISCRETE_MAJOR   0
#define DISCRETE_NAME    "DISCRETE LCD PORT"
#define DISCRETE_MODULE_VERSION "DISCRETE LCD PORT V0.1"
#define DISCRETE_ADDRESS	0xf1600000
#define DISCRETE_ADDRESS_RANGE	8

#define DISCRETE_BASE				0xbd
#define DISCRETE_VALUMEUP		_IOW(DISCRETE_BASE,0,8)
#define DISCRETE_VALUMEDOWN		_IOW(DISCRETE_BASE,1,8)
#define DISCRETE_VALUMEREAD		_IOW(DISCRETE_BASE,2,8)
#define DISCRETE_VALUEWRITE		_IOW(DISCRETE_BASE,3,8)

unsigned int *addr_discrete;

//Global variable
static int discreteport_usage = 0;
static int discreteport_major = 0;
static unsigned long current_valume = 0;

// define functions...
int discreteport_open(struct inode *minode, struct file *mfile) {
	if(discreteport_usage != 0) return -EBUSY;
	MOD_INC_USE_COUNT;
	discreteport_usage = 1;
	
	addr_discrete = (unsigned int *)(DISCRETE_ADDRESS);
	
	return 0;
}

int discreteport_release(struct inode *minode, struct file *mfile) {
	MOD_DEC_USE_COUNT;
	discreteport_usage = 0;
	return 0;
}

unsigned int Getvalumecode(int x)
{
	unsigned int code;
	switch (x) {
		case 1 : code = 0x80;break;
		case 2 : code = 0xc0;break;
		case 3 : code = 0xe0;break;
		case 4 : code = 0xf0;break;
		case 5 : code = 0xf8;break;
		case 6 : code = 0xfc;break;
		case 7 : code = 0xfe;break;
		case 8 : code = 0xff;break;
		default  : code = 0;break;
	}
	return code;
}



int discreteport_ioctl(struct inode *inode, struct file *file,unsigned int cmd,unsigned long gdata) {
	unsigned int hex_val;
	
	switch(cmd){
	case DISCRETE_VALUMEUP :
		if(current_valume <8){
			current_valume++;
			hex_val = Getvalumecode(current_valume);
			*addr_discrete = hex_val;
		}
		break;
	case DISCRETE_VALUMEDOWN :
		if(current_valume >= 1){
			current_valume--;
			hex_val = Getvalumecode(current_valume);
			*addr_discrete = hex_val;
		}
		break;
	case DISCRETE_VALUMEREAD :
		put_user(current_valume,(static unsigned long *)gdata);
		break;
	case DISCRETE_VALUEWRITE :
		get_user(current_valume,(static unsigned long *)gdata);
		printk("DISCRETE_VALUEWRITE : %d value setting \n",current_valume);
		hex_val = Getvalumecode(current_valume);
		*addr_discrete = hex_val;
		break;
	default:
		printk("driver : no such command!\n");
		return -ENOTTY;
	}

	return 0;
}

struct file_operations segment_fops = {
	ioctl: discreteport_ioctl,
	open: discreteport_open,
	release: discreteport_release,
};


int init_module(void) {

	int result;
	result = register_chrdev(DISCRETE_MAJOR,DISCRETE_NAME,&segment_fops);
	if(result<0)
	{
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	discreteport_major=result;

	
	if(!check_region(DISCRETE_ADDRESS,DISCRETE_ADDRESS_RANGE)) {
			request_region(DISCRETE_ADDRESS,DISCRETE_ADDRESS_RANGE,DISCRETE_NAME);
			printk("init module, discreteport major number : %d\n",result);
	}
	else printk("driver : unable to register this!\n");
	
	return 0;
}

void cleanup_module(void) {
	release_region(DISCRETE_ADDRESS,DISCRETE_ADDRESS_RANGE);

	if(unregister_chrdev(discreteport_major,DISCRETE_NAME))
		printk("driver : %s DRIVER CLEANUP Ok\n", DISCRETE_NAME);
	else
		printk("driver : %s DRIVER CLEANUP Failed\n", DISCRETE_NAME);

}
