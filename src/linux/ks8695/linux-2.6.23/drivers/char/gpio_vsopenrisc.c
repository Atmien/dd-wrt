#include <linux/version.h>
#include <linux/module.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/vsopenrisc.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/semaphore.h>
#include <asm/arch/regs-mem.h>
#include <asm/arch/regs-irq.h>

#if LINUX_VERSION_CODE < 0x020300
#include <linux/malloc.h>
#else
#include <linux/slab.h>
#endif

#if LINUX_VERSION_CODE < 0x020100
#define INIT_RET_TYPE	int
#define Module_init(a)
#elif LINUX_VERSION_CODE < 0x020300
#include <linux/init.h>
#define INIT_RET_TYPE	int
#define Module_init(a)	module_init(a)
#else
#include <linux/init.h>
#define INIT_RET_TYPE	static int __init
#define Module_init(a)	module_init(a)
#endif

#if LINUX_VERSION_CODE < 0x020100
#define Get_user(a,b)	a = get_user(b)
#else
#include <asm/uaccess.h>
#define Get_user(a,b)	get_user(a,b)
#endif

#define PROC_NAME_BTN_RST		   "vsopenrisc/reset"
#define PROC_NAME_LEDS			   "vsopenrisc/leds"
#define PROC_NAME_BUZZER		   "vsopenrisc/buzzer"
#define PROC_NAME_BUZZER_FRQ	   "vsopenrisc/buzzer_frq"

static char *table_proc_name[] = { "vsopenrisc/gpio_data"
								 , "vsopenrisc/gpio_ctrl"
								 , "vsopenrisc/gpio_irqmask"
								 , "vsopenrisc/gpio_change"
								 , "vsopenrisc/gpio_changes" };

static void gpio_set(unsigned long mask, unsigned long value);
static unsigned long gpio_get(void);

#if LINUX_VERSION_CODE < 0x020100
static struct symbol_table gpio_syms = {
#include <linux/symtab_begin.h>
	X(gpio_get),
	X(gpio_set),
#include <linux/symtab_end.h>
};
#else
EXPORT_SYMBOL(gpio_get);
EXPORT_SYMBOL(gpio_set);
#endif

struct proc_dir_entry *proc_init(char *name, read_proc_t *read_proc, 
									write_proc_t *write_proc, void *data);

/****************************************************************************/

static int  gpio_ioctl(struct inode * inode, struct file * file,
								unsigned int cmd, unsigned long arg);
unsigned int gpio_poll(struct file *filp, poll_table *wait);
/****************************************************************************/


struct file_operations gpio_fops = {
	ioctl: gpio_ioctl,	// gpio_ioctl
	poll: gpio_poll,	// gpio_poll 
};

// poll implementation stuff
wait_queue_head_t gpio_queue; // poll wait queue
int poll_flag = 0;	      // interrupt occurrence flag

#include <asm/hardware.h>
#include <asm/io.h>

#define GPIO_DATA_INIT		(0xc000UL)	// reset high & power led on & blue led on
#define GPIO_INPUTS			(0x800fUL)
//#define GPIO_OUTPUTS		(0x7C00UL)
#define GPIO_OUTPUTS		(MASK_LEDS | BUZZER_MASK)
#define GPIO_DATA		(*(volatile unsigned long *)(KS8695_GPIO_VA + KS8695_GPIO_DATA))
#define GPIO_MODE		(*(volatile unsigned long *)(KS8695_GPIO_VA + KS8695_GPIO_MODE))

#define GPIO_EXT_DATA_INIT	(0x0UL)
#define GPIO_EXT_INPUTS		(~0x0UL)
#define GPIO_EXT_OUTPUTS	(0x0UL)
#define GPIO_EXT_DATA		(*(volatile unsigned long *)(VSOPENRISC_VA_EPLD_GPIO_BASE + VSOPENRISC_GPIO_DATA))
#define GPIO_EXT_MODE		(*(volatile unsigned long *)(VSOPENRISC_VA_EPLD_GPIO_BASE + VSOPENRISC_GPIO_MODE))
#define GPIO_EXT_IRQMASK	(*(volatile unsigned long *)(VSOPENRISC_VA_EPLD_GPIO_BASE + VSOPENRISC_GPIO_IRQMASK))
#define GPIO_EXT_CHANGE		(*(volatile unsigned long *)(VSOPENRISC_VA_EPLD_GPIO_BASE + VSOPENRISC_GPIO_CHANGE))

static unsigned long gpio_ext_irq_changes[NUMBER_OF_GPIOS];
static unsigned long gpio_last_changed = 0;

#define BTN_RST_MASK		0x8000UL

#define BUZZER_MASK			0x0800UL

#define LED_POWER_MASK		0x4000UL
#define LED_BLUE_MASK		0x2000UL
#define LED_GREEN_MASK		0x0400UL

#define LED_POWER_STR_ON	"POWER"
#define LED_POWER_STR_OFF	"power"
#define LED_BLUE_STR_ON		"BLUE"
#define LED_BLUE_STR_OFF	"blue"
#define LED_GREEN_STR_ON	"GREEN"
#define LED_GREEN_STR_OFF	"green"

#define MASK_LEDS			(LED_POWER_MASK | LED_BLUE_MASK | LED_GREEN_MASK)
#define MASK_LEDS_OFF		0x0

#define BUZZER_STR_ON			"1"
#define BUZZER_STR_OFF			"0"

#define LEDS(v)				(((v & LED_POWER_MASK)?LED_POWER:0x00) | \
						 	((v & LED_BLUE_MASK)?LED_BLUE:0x00) | \
						 	((v & LED_GREEN_MASK)?LED_GREEN:0x00))

#define DO_RESET() \
	do { GPIO_MODE |= BTN_RST_MASK; \
		GPIO_DATA &= ~BTN_RST_MASK; \
		udelay(1000); \
		GPIO_MODE &= ~BTN_RST_MASK; } while(0)
	
#define BUTTON_RESET()		(!(GPIO_DATA & BTN_RST_MASK))

/****************************************************************************/
/************************** Buzzer stuff ************************************/
/****************************************************************************/
#define FREQ_MAX 2147483648UL	//!< maximum frequency value defined by signed long value used by chedule_timeout system call (2^31)
#define FREQ_MIN 10UL		//!< minimum frequency value 
unsigned long buzzer_freq = 0;	//!< buzzer frequency

static struct task_struct *freq_thread = NULL; //!< buzzer thread handle

/* prototypes */
static void set_buzzer(int mode);
static int buzzer_thread(void *unused);

/****************************************************************************/

static void
gpio_set(unsigned long mask, unsigned long value)
{
	unsigned long flags;
	unsigned long val;

	mask &= GPIO_OUTPUTS;
	
	save_flags(flags); cli();
	val = GPIO_DATA;
	val &= ~mask;
	val |= (value & mask);
	GPIO_DATA = val;
	restore_flags(flags);
}

static unsigned long
gpio_get(void)
{
	unsigned long value;

	value = GPIO_DATA;
	return value;
}

static inline unsigned long led2reg(unsigned long val)
{
	unsigned long val2 = MASK_LEDS_OFF;

	if (val & LED_POWER)
		val2 |= LED_POWER_MASK;
	if (val & LED_BLUE)
		val2 |= LED_BLUE_MASK;
	if (val & LED_GREEN)
		val2 |= LED_GREEN_MASK;

	return val2;
}

/****************************************************************************/

static void
gpio_ext_set(unsigned long mask, unsigned long value)
{
	unsigned long flags;
	unsigned long val;

	save_flags(flags); cli();
	val = GPIO_EXT_DATA;
	val &= ~mask;
	val |= (value & mask);
	GPIO_EXT_DATA = val;
	restore_flags(flags);
}

static unsigned long
gpio_ext_get(void)
{
	unsigned long value;

	value = (GPIO_EXT_DATA & 0xFF);	// only 8bit so far
	return value;
}

static void
gpio_ext_set_mode(unsigned long mask, unsigned long value)
{
	unsigned long flags;
	unsigned long val;

	save_flags(flags); cli();
	val = GPIO_EXT_MODE;
	val &= ~mask;
	val |= (value & mask);
	GPIO_EXT_MODE = val;
	restore_flags(flags);
}
	
static unsigned long
gpio_ext_get_mode(void)
{
	unsigned long value;

	value = (GPIO_EXT_MODE & 0xFF);	// only 8bit so far
	return value;
}

static void
gpio_ext_set_irqmask(unsigned long mask, unsigned long value)
{
	unsigned long flags;
	unsigned long val;

	save_flags(flags); cli();
	val = GPIO_EXT_IRQMASK;
	val &= ~mask;
	val |= (value & mask);
	GPIO_EXT_IRQMASK = val;
	restore_flags(flags);
}
	
static unsigned long
gpio_ext_get_irqmask(void)
{
	unsigned long value;

	value = (GPIO_EXT_IRQMASK & 0xFF);	// only 8bit so far
	return value;
}

static unsigned long
gpio_ext_get_change(void)
{
	unsigned long value;

	if (gpio_last_changed)
	{
		value = gpio_last_changed;
		gpio_last_changed = 0;
	}
	else
		value = (GPIO_EXT_CHANGE & 0xFF); // only 8bit so far
	
	return value;
}

static int
gpio_ioctl(
	struct inode * inode,
	struct file * file,
	unsigned int cmd,
	unsigned long arg)
{
	unsigned long val;

	switch(cmd)
	{
		case GPIO_CMD_SET_BTN_RST:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			if (val)
				DO_RESET();
			break;
		case GPIO_CMD_GET_BTN_RST:
			val = BUTTON_RESET();
			if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_SET_LEDS:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			gpio_set(MASK_LEDS, led2reg(val));
			break;
		case GPIO_CMD_GET_LEDS:
			val = LEDS(gpio_get());
			if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_SET_LED_POWER:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			gpio_set(LED_POWER_MASK, val?LED_POWER_MASK:0UL);
			break;
		case GPIO_CMD_SET_LED_BLUE:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			gpio_set(LED_BLUE_MASK, val?LED_BLUE_MASK:0UL);
			break;
		case GPIO_CMD_SET_LED_GREEN:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			gpio_set(LED_GREEN_MASK, val?LED_GREEN_MASK:0UL);
			break;

		// buzzer
		case GPIO_CMD_SET_BUZZER:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;
			if (val == 1)
				set_buzzer(1);
			else if(val == 0)
				set_buzzer(0);
			else
				return -EINVAL;
			break;
		case GPIO_CMD_GET_BUZZER:
			val = (gpio_get() & BUZZER_MASK)?1:0;
			if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_SET_BUZZER_FRQ:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(val)))
	        	return -EFAULT;

			if((val < FREQ_MIN) || (val > FREQ_MAX))
				return -EINVAL;

			// check if thread has been already activated
			if(freq_thread != NULL)
			{
				kthread_stop(freq_thread);
				freq_thread = NULL;
			}

			buzzer_freq = val;
			freq_thread = kthread_run(buzzer_thread, NULL, "buzzer thread");
			if(IS_ERR(freq_thread))
			{
				printk("Error starting kthread\n");
				freq_thread = NULL;
			}
			break;
		case GPIO_CMD_GET_BUZZER_FRQ:
			val = buzzer_freq;
			if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;

		// external GPIOs
		case GPIO_CMD_SET:
			{
				struct gpio_struct set;
				
    			if (copy_from_user(&set, (struct gpio_struct*)arg, sizeof(set)))
		        	return -EFAULT;
				
				gpio_ext_set(set.mask, set.value);
			}
			break;
		case GPIO_CMD_GET:
			val = gpio_ext_get();
    		if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_SET_CTRL:
   			{
				struct gpio_struct set;
				
    			if (copy_from_user(&set, (struct gpio_struct*)arg, sizeof(set)))
		        	return -EFAULT;
				
			// relays are not to be switched to input and isolated inputs are not to be switched to output
			if(vs_sysid == 2)
			{
				if((set.mask & GPIO_BIT_0 && set.value & GPIO_BIT_0) || (set.mask & GPIO_BIT_1 && set.value & GPIO_BIT_1) ||
				    (set.mask & GPIO_BIT_6 && !(set.value & GPIO_BIT_6)) || (set.mask & GPIO_BIT_7 && !(set.value & GPIO_BIT_7)))
					return -EINVAL;
			}	
				gpio_ext_set_mode(set.mask, set.value);
			}
			break;
		case GPIO_CMD_GET_CTRL:
			val = gpio_ext_get_mode();
    		if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_SET_IRQMASK:
   			{
				struct gpio_struct set;
				
    			if (copy_from_user(&set, (struct gpio_struct*)arg, sizeof(set)))
		        	return -EFAULT;
				
				gpio_ext_set_irqmask(set.mask, set.value);
			}
			break;
		case GPIO_CMD_GET_IRQMASK:
			val = gpio_ext_get_irqmask();
    		if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_GET_CHANGE:
			val = gpio_ext_get_change();
    		if (copy_to_user((unsigned long *)arg, &val, sizeof(val)))
		       	return -EFAULT;
			break;
		case GPIO_CMD_GET_CHANGES:
   			if (copy_from_user(&val, (unsigned long*)arg, sizeof(unsigned long)))
	        	return -EFAULT;
			if (val >= NUMBER_OF_GPIOS)
				return -EFAULT;
    		if (copy_to_user((unsigned long*)arg, &gpio_ext_irq_changes[val], sizeof(gpio_ext_irq_changes[0])))
		       	return -EFAULT;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

// poll implementation
unsigned int gpio_poll(struct file *filp, poll_table *wait)
{
  unsigned int mask = 0;
  
  // register wait queue
  poll_wait(filp, &gpio_queue, wait);

  // if interrupt occurred set the mask
  // to indicate that the read file
  // descriptor is ready
  if(poll_flag == 1)
  {
    mask |= POLLIN | POLLRDNORM;
    poll_flag = 0;
  }

  return mask;
}
/****************************************************************************/

// Support for the gpios in /proc

static int proc_gpio_read(char *buffer, char **buffer_location, off_t offset,
					int buffer_length, int *eof, void *data)
{
	int i, ret = 0, cmd = (int)data;
	unsigned long val = 0;
	char temp[32];

	switch (cmd)
	{
		case GPIO_VAL_DATA:
			val = gpio_ext_get();
			break;
		case GPIO_VAL_CTRL:
			val = gpio_ext_get_mode();
			break;
		case GPIO_VAL_IRQMASK:
			val = gpio_ext_get_irqmask();
			break;
		case GPIO_VAL_CHANGE:
			val = gpio_ext_get_change();
			break;
		case GPIO_VAL_CHANGES:
			buffer[0] = '\0';
			for (i = 0; i < NUMBER_OF_GPIOS; i++)
			{
				sprintf(temp, "GPIO %d: %lu\n", i, gpio_ext_irq_changes[i]);
				strcat(buffer, temp);
			}
			ret = strlen(buffer);
			offset = 1;
			break;
		default:
			printk(KERN_ERR "Unhandled proc read cmd %d\n", cmd);
			break;
	}

	if (!offset)
	{
		ret = sprintf(buffer, "0x%08lX\n", val);
		if (cmd != GPIO_CMD_GET_CHANGE)
			ret = strlen(strcat(buffer, "\nSetting Values:\n---------------\n<mask> <value>\n"));
	}

	return ret;
}

static int proc_gpio_write(struct file *file, const char *buffer, 
					unsigned long count, void *data)
{
	char temp[256], *tmp;
	int size = sizeof(temp) - 1, cmd = (int)data;
	unsigned long val, mask;

	if (count < size)
		size = count;

	if (copy_from_user(temp, buffer, size))
		return -EFAULT;
	if (temp[size - 1] == '\n')
		size--;
	temp[size] = '\0';
	
	tmp = strchr(temp, ' ');
	if (tmp != NULL)
	{
		*tmp = '\0';
		if (++tmp >= temp + size)
			return count;
		
		mask = simple_strtoul(temp, NULL, 0);
		val = simple_strtoul(tmp, NULL, 0);
		
		switch (cmd)
		{
			case GPIO_VAL_DATA:
				gpio_ext_set(mask, val);
				break;
			case GPIO_VAL_CTRL:
				gpio_ext_set_mode(mask, val);
				break;
			case GPIO_VAL_IRQMASK:
				gpio_ext_set_irqmask(mask, val);
				break;
			case GPIO_VAL_CHANGE:
			case GPIO_VAL_CHANGES:
				break;
			default:
				printk(KERN_ERR "Unhandled proc write cmd %d\n", cmd);
				break;
		}
	}

	return count;
}

static int proc_btn_rst_read(char *buffer, char **buffer_location, off_t offset,
					int buffer_length, int *eof, void *data)
{
	int ret = 0;

	if (!offset)
	{
		if (BUTTON_RESET())
			strcpy(buffer, "1\n");
		else
			strcpy(buffer, "0\n");
		ret = 2;
	}

	return ret;
}

static int proc_btn_rst_write(struct file *file, const char *buffer, 
					unsigned long count, void *data)
{
	char temp[32];
	int size = sizeof(temp) - 1;

	if (count < size)
		size = count;

	if (copy_from_user(temp, buffer, size))
		return -EFAULT;

	if (temp[0] == '1')
		DO_RESET();

	return count;
}

static struct
{
	char on[16];
	char off[16];
	unsigned long bit;
} led_str[] =
{
	 { LED_POWER_STR_ON, LED_POWER_STR_OFF, LED_POWER_MASK }
	,{ LED_BLUE_STR_ON, LED_BLUE_STR_OFF, LED_BLUE_MASK }
	,{ LED_GREEN_STR_ON, LED_GREEN_STR_OFF, LED_GREEN_MASK }
};
static int led_str_size = sizeof(led_str) / sizeof(led_str[0]);


static int proc_leds_read(char *buffer, char **buffer_location, off_t offset,
					int buffer_length, int *eof, void *data)
{
	int ret = 0;

	if (!offset)
	{
		unsigned long val = gpio_get();
		ret = sprintf(buffer, "0x%02X\n%s %s %s\n", LEDS(val),
						(val & LED_BLUE_MASK)?LED_BLUE_STR_ON:LED_BLUE_STR_OFF,
						(val & LED_POWER_MASK)?LED_POWER_STR_ON:LED_POWER_STR_OFF,
						(val & LED_GREEN_MASK)?LED_GREEN_STR_ON:LED_GREEN_STR_OFF);
	}

	return ret;
}

static int proc_leds_write(struct file *file, const char *buffer, 
					unsigned long count, void *data)
{
	char temp[256], *tmp;
	int i, size = sizeof(temp) - 1;
	unsigned long val, val2 = MASK_LEDS_OFF, mask = 0;

	if (count < size)
		size = count;

	if (copy_from_user(temp, buffer, size))
		return -EFAULT;
	if (temp[size - 1] == '\n')
		size--;
	temp[size] = '\0';
	
	tmp = strstr(temp, "0x");
	if (tmp != NULL)
	{
		val = simple_strtoul(tmp, NULL, 0);
		gpio_set(MASK_LEDS, led2reg(val));
		return count;
	}

	for (i = 0; i < led_str_size; i++)
	{
		if (strstr(temp, led_str[i].on))
		{
			mask |= led_str[i].bit;
			val2 |= led_str[i].bit;
		}
		if (strstr(temp, led_str[i].off))
		{
			mask |= led_str[i].bit;
			val2 &= ~led_str[i].bit;
		}
	}
	
	gpio_set(mask, val2);
	
	return count;
}

/****************************************************************************/
static void set_buzzer(int mode)
{
	
	if (mode) // activate buzzer
	{
		// check if thread has been already activated
		if(freq_thread != NULL)
		{
			kthread_stop(freq_thread);
			freq_thread = NULL;
		}

		gpio_set(BUZZER_MASK, BUZZER_MASK);
	}
	else	// deactivate buzzer
	{
		// check if thread has been already activated
		if(freq_thread != NULL)
		{
			kthread_stop(freq_thread);
			freq_thread = NULL;
		}

		gpio_set(BUZZER_MASK, 0UL);
	}
}

static int proc_buzzer_read_frq(char *buffer, char **buffer_location, off_t offset,
					int buffer_length, int *eof, void *data)
{
	int ret = 0;

	if (!offset)
	{
		ret = sprintf(buffer, "Buzzer frequncy: 0x%08lX ms\n", buzzer_freq);
	}

	return ret;
}

static int proc_buzzer_write_frq(struct file *file, const char *buffer, 
					unsigned long count, void *data)
{
	char temp[32];
	unsigned long val;
	int size = sizeof(temp) - 1;

	if (count < size)
		size = count;

	if (copy_from_user(temp, buffer, size))
		return -EFAULT;
	
	if (temp[size - 1] == '\n')
		size--;
	temp[size] = '\0';

	if(strlen(temp) == 10)  // 32-bit value entered
	{
		val = simple_strtoul(temp, NULL, 0);
		if((val < FREQ_MIN) || (val > FREQ_MAX))
		{
			printk(KERN_INFO "Buzzer: The frequency value is out of range (min freq = 0x%08lX and max freq = 0x%08lX)\n", FREQ_MIN, FREQ_MAX);
			return count;
		}

		// check if thread has been already activated
		if(freq_thread != NULL)
		{
			kthread_stop(freq_thread);
			freq_thread = NULL;
		}

		buzzer_freq = val;
		freq_thread = kthread_run(buzzer_thread, NULL, "buzzer thread");
		if(IS_ERR(freq_thread))
		{
			printk("Error starting ktread\n");
			freq_thread = NULL;
		}
	}
	else
	{
		printk(KERN_INFO "Buzzer: wrong value\n");
	}

	return count;
}

static int proc_buzzer_read(char *buffer, char **buffer_location, off_t offset,
					int buffer_length, int *eof, void *data)
{
	int ret = 0;

	if (!offset)
	{
		unsigned long val = gpio_get();
		ret = sprintf(buffer, "Buzzer: %s\n", (val & BUZZER_MASK)?BUZZER_STR_ON:BUZZER_STR_OFF);
	}

	return ret;
}

static int proc_buzzer_write(struct file *file, const char *buffer, 
					unsigned long count, void *data)
{
	char temp[32];
	int size = sizeof(temp) - 1;

	if (count < size)
		size = count;

	if (copy_from_user(temp, buffer, size))
		return -EFAULT;
	
	if (temp[size - 1] == '\n')
		size--;
	temp[size] = '\0';

	if (temp[0] == '1') // activate buzzer
	{
		set_buzzer(1);
	}
	else if(temp[0] == '0')	// deactivate buzzer
	{
		set_buzzer(0);
	}
	else
	{
		printk(KERN_INFO "Buzzer: wrong value\n");
	}
		

	return count;
}
/****************************************************************************/

struct proc_dir_entry *proc_init(char *name, read_proc_t *read_proc, 
									write_proc_t *write_proc, void *data)
{
	struct proc_dir_entry *proc_entry = create_proc_entry(name, 0644, NULL);

	if (proc_entry == NULL)
	{
		//remove_proc_entry(name, &proc_root);
		remove_proc_entry(name, 0);
		printk(KERN_ALERT "Could not initialize /proc/%s\n", name);
		return NULL;
	}
	else
	{
		proc_entry->read_proc = read_proc;
		proc_entry->write_proc = write_proc;
		proc_entry->owner = THIS_MODULE;
		proc_entry->mode = S_IFREG | S_IRUGO;
		proc_entry->uid = 0;
		proc_entry->gid = 0;
		proc_entry->size = 0;
		proc_entry->data = data;
		//printk(KERN_INFO "/proc/%s created\n", name);
	}
	
	return proc_entry;
}

/****************************************************************************/

static irqreturn_t gpio_ext_irq_handler(int irq, void *dev_id)
{
	int i, changed;
	
	changed = (GPIO_EXT_CHANGE & 0xFF);	// only 8bit so far
	gpio_last_changed |= changed; 
	
	for (i = 0; i < NUMBER_OF_GPIOS; i++)
	{
		if (changed & (1 << i))
			gpio_ext_irq_changes[i]++;
	}

	// poll implementation: interrupt service routine part
	// indicate interrupt occurrence and wake up the thread
	poll_flag = 1;
	wake_up_interruptible(&gpio_queue);

	return 0;
}

/****************************************************************************/
static struct miscdevice gpio_vsopenrisc_miscdev = {
	.minor		= 0,
	.name		= "gpio",
	.fops		= &gpio_fops,
};


static int __init gpio_vsopenrisc_probe(struct platform_device *pdev)
{
	int res;

	gpio_vsopenrisc_miscdev.parent = &pdev->dev;

	res = misc_register(&gpio_vsopenrisc_miscdev);
	if (res)
		return res;

	printk(KERN_INFO "gpio_vsopnerisc: driver registered\n");

	return 0;
}

static int __exit gpio_vsopenrisc_remove(struct platform_device *pdev)
{
	int res;

	res = misc_deregister(&gpio_vsopenrisc_miscdev);
	if (!res)
		gpio_vsopenrisc_miscdev.parent = NULL;

	return res;
}

static void gpio_vsopenrisc_shutdown(struct platform_device *pdev)
{
}

static int gpio_vsopenrisc_suspend(struct platform_device *pdev, pm_message_t message)
{
	return 0;
}

static int gpio_vsopenrisc_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver gpio_vsopenrisc_driver = {
	.probe		= gpio_vsopenrisc_probe,
	.remove		= __exit_p(gpio_vsopenrisc_remove),
	.shutdown	= gpio_vsopenrisc_shutdown,
	.suspend	= gpio_vsopenrisc_suspend,
	.resume		= gpio_vsopenrisc_resume,
	.driver		= {
		.name	= "gpio_vsopenrisc",
		.owner	= THIS_MODULE,
	},
};

INIT_RET_TYPE gpio_init(void)
{
	int i;

	printk(KERN_INFO "gpio_vsopenrisc: GPIO driver.\n");

	// initialize poll queue
	init_waitqueue_head(&gpio_queue);

	memset(gpio_ext_irq_changes, 0, sizeof(gpio_ext_irq_changes));

	GPIO_DATA = GPIO_DATA_INIT;
	GPIO_EXT_DATA = GPIO_EXT_DATA_INIT;

	/* Enable inputs */
	GPIO_MODE &= ~GPIO_INPUTS;
	GPIO_EXT_MODE &= ~GPIO_EXT_INPUTS;

	/* Enable outputs */
	GPIO_MODE |= GPIO_OUTPUTS;
	GPIO_EXT_MODE |= GPIO_EXT_OUTPUTS;

	/* Disable all interrupts */
	GPIO_EXT_IRQMASK = 0x00;

	if (request_irq(KS8695_INTEPLD_GPIO, gpio_ext_irq_handler, IRQF_SHARED, "gpio_vsopenrisc", (void*)1) < 0)
		printk(KERN_ERR "%s(%d): request_irq() couldn't register the interrupt for the gpios\n",
				__FILE__, __LINE__);

	if (register_chrdev(VSOPENRISC_GPIO_MAJOR, "gpio",  &gpio_fops) < 0) {
		printk("%s(%d): gpio_init() can't get Major %d\n",
				__FILE__, __LINE__, VSOPENRISC_GPIO_MAJOR);
		return(-EBUSY);
	} 

#if LINUX_VERSION_CODE < 0x020100
	register_symtab(&gpio_syms);
#endif

	proc_init(PROC_NAME_BTN_RST, proc_btn_rst_read, proc_btn_rst_write, NULL);
	proc_init(PROC_NAME_LEDS, proc_leds_read, proc_leds_write, NULL);
	proc_init(PROC_NAME_BUZZER, proc_buzzer_read, proc_buzzer_write, NULL);
	proc_init(PROC_NAME_BUZZER_FRQ, proc_buzzer_read_frq, proc_buzzer_write_frq, NULL);

	for (i = 0; i < GPIO_VAL_MAX; i++)
	{
		proc_init(table_proc_name[i], proc_gpio_read, proc_gpio_write, (void*)i);
	}

	//return(0);
	return platform_driver_register(&gpio_vsopenrisc_driver);
}

Module_init(gpio_init);

/* this is the thread function that we are executing */
static int buzzer_thread(void *unused)
{
	int flag = 0;

	__set_current_state(TASK_RUNNING);

	/* an endless loop in which we are doing our work */
	for(;;)
	{
		/* fall asleep for required amoutn of ms */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(buzzer_freq/10);

		/* We need to do a memory barrier here to be sure that
		   the flags are visible on all CPUs. 
		*/
		 mb();
		 
		/* here we are back from sleep, either due to the timeout
		   (one second), or because we caught a signal.
		*/
		if (kthread_should_stop())
		{
			/* we received a request to terminate ourself */
			gpio_set(BUZZER_MASK, 0UL);
			buzzer_freq = 0UL;
			break;    
		}
		
		/* this is normal work to do */
		if(flag == 0)
		{
			gpio_set(BUZZER_MASK, BUZZER_MASK);
			flag = 1;
		}
		else
		{
			gpio_set(BUZZER_MASK, 0UL);
			flag = 0;
		}
	}
	/* here we go only in case of termination of the thread */

	return 0;
}
