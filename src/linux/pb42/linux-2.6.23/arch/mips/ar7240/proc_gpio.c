/*
 * proc_gpio: AR5315 GPIO pins in /proc/gpio/
 * by olg 
 * modification for AR7240 support by Sebastian Gottschall <s.gottschall@newmedia-net.de>
 * GPL'ed
 * some code stolen from Yoshinori Sato <ysato@users.sourceforge.jp>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for copy_from_user */

#include "ar7240.h"

#define PROCFS_MAX_SIZE 64
extern const char *get_arch_type (void);
struct proc_dir_entry *proc_gpio, *gpio_dir;

//Masks for data exchange through "void *data" pointer
#define GPIO_IN (1<<5)
#define GPIO_OUT (1<<6)
#define GPIO_DIR (1<<7)
#define PIN_MASK 0x1f
#define GPIO_CR_M(x)                (1 << (x))	/* mask for i/o */


static void cleanup_proc (void);

//The buffer used to store the data returned by the proc file
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;


static int
gpio_proc_read (char *buf, char **start, off_t offset,
		int len, int *eof, void *data)
{
  u32 reg = 0;
      if ((unsigned int) data & GPIO_IN)
	reg = ar7240_reg_rd(AR7240_GPIO_IN);
      if ((unsigned int) data & GPIO_OUT)
	reg = ar7240_reg_rd(AR7240_GPIO_OUT);
      if ((unsigned int) data & GPIO_DIR)
	reg = ar7240_reg_rd(AR7240_GPIO_OE);

  if (GPIO_CR_M (((unsigned int) data) & PIN_MASK) & reg)
    buf[0] = '1';
  else
    buf[0] = '0';
  buf[1] = 0;

  *eof = 1;

  return (2);

}

static int
gpio_proc_info_read (char *buf, char **start, off_t offset,
		     int len, int *eof, void *data)
{
  *eof = 1;
      return (sprintf
	      (buf, "GPIO_IN   %#08X \nGPIO_OUT  %#08X \nGPIO_DIR  %#08X \n",
	       ar7240_reg_rd(AR7240_GPIO_IN), ar7240_reg_rd(AR7240_GPIO_OUT),
	       ar7240_reg_rd(AR7240_GPIO_OE)));
}

static int
gpio_proc_write (struct file *file, const char *buffer, unsigned long count,
		 void *data)
{
  u32 reg = 0;

  /* get buffer size */
  procfs_buffer_size = count;
  if (procfs_buffer_size > PROCFS_MAX_SIZE)
    {
      procfs_buffer_size = PROCFS_MAX_SIZE;
    }
  /* write data to the buffer */
  if (copy_from_user (procfs_buffer, buffer, procfs_buffer_size))
    {
      return -EFAULT;
    }

  procfs_buffer[procfs_buffer_size] = 0;

      if ((unsigned int) data & GPIO_IN)
	reg = ar7240_reg_rd(AR7240_GPIO_IN);
      if ((unsigned int) data & GPIO_OUT)
	reg = ar7240_reg_rd(AR7240_GPIO_OUT);
      if ((unsigned int) data & GPIO_DIR)
	reg = ar7240_reg_rd(AR7240_GPIO_OE);

  if (procfs_buffer[0] == '0' || procfs_buffer[0] == 'i')
    reg = reg & ~(GPIO_CR_M (((unsigned int) data) & PIN_MASK));
  if (procfs_buffer[0] == '1' || procfs_buffer[0] == 'o')
    reg = reg | GPIO_CR_M (((unsigned int) data) & PIN_MASK);


      if ((unsigned int) data & GPIO_IN)
	{
          ar7240_reg_wr(AR7240_GPIO_IN, reg);
	}
      if ((unsigned int) data & GPIO_OUT)
	{
          ar7240_reg_wr(AR7240_GPIO_OUT, reg);
	}
      if ((unsigned int) data & GPIO_DIR)
	{
          ar7240_reg_wr(AR7240_GPIO_OE, reg);
	}

  return procfs_buffer_size;
}

void ar7100_set_gpio(int gpio, int val)
{
u32 reg = ar7240_reg_rd(AR7240_GPIO_OE);
reg |= 1<<gpio;
ar7240_reg_wr(AR7240_GPIO_OE, reg);
(void)ar7240_reg_rd(AR7240_GPIO_OE); /* flush write to hardware */
reg = ar7240_reg_rd(AR7240_GPIO_OUT);
if (val)
    reg|=1<<gpio;
else
    reg&=~(1<<gpio);
ar7240_reg_wr(AR7240_GPIO_OUT, reg);
}

int ar7100_get_gpio(int gpio)
{
u32 reg = ar7240_reg_rd(AR7240_GPIO_OE);
reg&=~(1<<gpio);
ar7240_reg_wr(AR7240_GPIO_OE, reg);
reg = ar7240_reg_rd(AR7240_GPIO_IN);
if (reg&(1<<gpio))
    return 1;
else
    return 0;
}
EXPORT_SYMBOL(ar7100_set_gpio);
EXPORT_SYMBOL(ar7100_get_gpio);


static __init int
register_proc (void)
{
  unsigned char i, flag = 0;
  char proc_name[22];
  int gpiocount = 22;

  /* create directory gpio */
  gpio_dir = proc_mkdir ("gpio", NULL);
  if (gpio_dir == NULL)
    goto fault;
  gpio_dir->owner = THIS_MODULE;

  for (i = 0; i < gpiocount * 3; i++)	//create for every GPIO "x_in"," x_out" and "x_dir"
    {
      if (i / gpiocount == 0)
	{
	  flag = GPIO_IN;
	  sprintf (proc_name, "%i_in", i);
	}
      if (i / gpiocount == 1)
	{
	  flag = GPIO_OUT;
	  sprintf (proc_name, "%i_out", i % gpiocount);
	}
      if (i / gpiocount == 2)
	{
	  flag = GPIO_DIR;
	  sprintf (proc_name, "%i_dir", i % gpiocount);
	}

      proc_gpio = create_proc_entry (proc_name, S_IRUGO, gpio_dir);
      if (proc_gpio)
	{
	  proc_gpio->read_proc = gpio_proc_read;
	  proc_gpio->write_proc = gpio_proc_write;
	  proc_gpio->owner = THIS_MODULE;
	  proc_gpio->data = ((i % gpiocount) | flag);
	}
      else
	goto fault;

    }

  proc_gpio = create_proc_entry ("info", S_IRUGO, gpio_dir);
  if (proc_gpio)
    {
      proc_gpio->read_proc = gpio_proc_info_read;
      proc_gpio->owner = THIS_MODULE;
    }
  else
    goto fault;

  printk (KERN_NOTICE "gpio_proc: module loaded and /proc/gpio/ created\n");
  return 0;

fault:
  cleanup_proc ();
  return -EFAULT;
}

static void
cleanup_proc (void)
{
  unsigned char i;
  char proc_name[22];
  int gpiocount=22;

  for (i = 0; i < gpiocount; i++)
    {
      sprintf (proc_name, "%i_in", i);
      remove_proc_entry (proc_name, gpio_dir);
      sprintf (proc_name, "%i_out", i);
      remove_proc_entry (proc_name, gpio_dir);
      sprintf (proc_name, "%i_dir", i);
      remove_proc_entry (proc_name, gpio_dir);
    }
  remove_proc_entry ("info", gpio_dir);
  remove_proc_entry ("gpio", NULL);
  printk (KERN_INFO "gpio_proc: unloaded and /proc/gpio/ removed\n");

}


module_init (register_proc);
module_exit (cleanup_proc);

MODULE_AUTHOR ("Sebastian Gottschall");
MODULE_DESCRIPTION ("AR7240 GPIO pins in /proc/gpio/");
MODULE_LICENSE ("GPL");
