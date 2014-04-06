
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>

#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/param.h>
#include <linux/timer.h>



#define TIMER_INTERVAL			50
#define IRQ_BUTTON		IRQ_GPIO(16)


static void button_interrupt(int irq, void *dev_id, struct pt_regs *regs);
static struct timer_list mygpiotimer;
static int GPIO_MAJOR = 0;
static int pressed = 0;
static int id;
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

int led = 0;

static void button_interrupt(int irq, void *dev_id, struct pt_regs *regs){
	wake_up_interruptible(&wait_queue);
}

void myGpiofunction(unsigned long data){
	if (pressed == 1) kill_proc(id,SIGUSR2,1);
	pressed = 0;
	mygpiotimer.expires = jiffies + TIMER_INTERVAL;
	add_timer(&mygpiotimer);
}

ssize_t gpio_write(struct file *inode, const char * gdata, size_t length, loff_t * off_what){
	get_user(id,(int *)gdata);
	init_timer(&mygpiotimer);
	mygpiotimer.expires = jiffies + TIMER_INTERVAL;
	mygpiotimer.function = &myGpiofunction;
	add_timer(&mygpiotimer);
	return length;
}

ssize_t gpio_read(struct file *inode, char *gdata, size_t length, loff_t *off_what){
	write_led();
	return length;
}


static int gpio_open(struct inode *inode, struct file *filp){
	int res;
	unsigned int gafr;

	gafr = 0x3;
	GAFR0_U &= ~gafr;


        printk("IRQ_BUTTON = %d\n",IRQ_BUTTON);
        printk("IRQ_TO_GPIO = %d\n",IRQ_TO_GPIO_2_80(IRQ_BUTTON));

	
	GPDR(IRQ_TO_GPIO_2_80(IRQ_BUTTON)) &= ~GPIO_bit(IRQ_TO_GPIO_2_80(IRQ_BUTTON));
	set_GPIO_IRQ_edge(IRQ_TO_GPIO_2_80(IRQ_BUTTON),GPIO_FALLING_EDGE);


	res = request_irq(IRQ_BUTTON,&button_interrupt,SA_INTERRUPT,"Button",NULL);
	if(res < 0)
		printk(KERN_ERR "%s: Request for IRQ %d failed\n",__FUNCTION__,IRQ_BUTTON);
	else {
		enable_irq(IRQ_BUTTON);
	}

	MOD_INC_USE_COUNT;
	return 0;
}


static int gpio_release(struct inode *inode, struct file *filp)
{
	free_irq(IRQ_BUTTON,NULL);
	disable_irq(IRQ_BUTTON);
	del_timer(&mygpiotimer);
	MOD_DEC_USE_COUNT;
	return 0;
}

static int write_led()
{		
	unsigned int gafr;

	gafr = 0x3;
	GAFR0_U &= ~gafr;
	
	if(led == 0)
	{
		GPSR0 = GPSR0 | 0x20000; 
		led =1;
	}else{
		GPCR0 = GPCR0 | 0x20000;
		led =0;
	}
		
}


static struct file_operations gpio_fops = {
	read: gpio_read,
	write: gpio_write,
	open:	gpio_open,
	release:	gpio_release,
};

int init_module(void){
	int result;
	result = register_chrdev(GPIO_MAJOR,"GPIO INTERRUPT",&gpio_fops);

	if(result < 0) {
		printk(KERN_WARNING"Can't get major %d\n",GPIO_MAJOR);
		return result;
	}

	if(GPIO_MAJOR == 0) GPIO_MAJOR = result;
	printk("init module, GPIO major number : %d\n",result);

	return 0;
}

void cleanup_module(void){
	unregister_chrdev(GPIO_MAJOR,"GPIO INTERRUPT");
	return;
}
	
