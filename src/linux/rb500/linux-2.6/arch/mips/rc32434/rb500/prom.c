/*
* prom.c 
**********************************************************************
* P . Sadik Oct 10, 2003
*
* Started change log
* idt_cpu_freq is make a kernel configuration parameter
* idt_cpu_freq is exported so that other modules can use it.
* Code cleanup
**********************************************************************
* P. Sadik Oct 20, 2003
*
* Removed NVRAM code from here, since they are already available under
* nvram directory.
* Added serial port initialisation.
**********************************************************************
**********************************************************************
* P. Sadik Oct 30, 2003
*
* Added reset_cons_port
**********************************************************************

  P.Christeas, 2005-2006
  Port to 2.6, add 2.6 cmdline parsing

*/

#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/console.h>
#include <asm/bootinfo.h>
#include <linux/bootmem.h>
#include <linux/ioport.h>
#include <linux/blkdev.h>
#include <asm/rc32434/ddr.h>

#define PROM_ENTRY(x)   (0xbfc00000+((x)*8))
extern void __init setup_serial_port(void);
extern void cons_putc(char c);
extern void cons_puts(char *s);

unsigned int idt_cpu_freq = 132000000;
EXPORT_SYMBOL(idt_cpu_freq);
unsigned int board_type = 500;
EXPORT_SYMBOL(board_type);
unsigned int gpio_bootup_state = 0;
EXPORT_SYMBOL(gpio_bootup_state);


char mips_mac_address[18] = "08:00:06:05:40:01";
EXPORT_SYMBOL(mips_mac_address);

/* what to append to cmdline when button is [not] pressed */
#define GPIO_INIT_NOBUTTON ""
#define GPIO_INIT_BUTTON   " 2"

/* Board specific power off */
// void (*pm_power_off)(void);
// EXPORT_SYMBOL(pm_power_off);

#ifdef CONFIG_MIKROTIK_RB500
unsigned soft_reboot = 0;
EXPORT_SYMBOL(soft_reboot);
#endif

#define SR_NMI			0x00180000      /* NMI */
#define SERIAL_SPEED_ENTRY	0x00000001

#ifdef CONFIG_REMOTE_DEBUG
extern int remote_debug;
#endif

extern unsigned long mips_machgroup;
extern unsigned long mips_machtype;

/* RB500 memory map -- please fix it */

#if 0
#define RAM_FIRST       0x80000400  /* Leave room for interrupt vectors */
#define RAM_SIZE        (32*1024*1024 - 512)
//#define RAM_END         (0x80000000 + RAM_SIZE)     
struct resource rc32434_res_ram = {
	"RAM",
	0,
	RAM_SIZE,
	IORESOURCE_MEM
};
#endif

#define FREQ_TAG   "HZ="
#define GPIO_TAG   "gpio="
#define KMAC_TAG   "kmac="
#define MEM_TAG	   "mem="
#define BOARD_TAG  "board="
//#define IGNORE_CMDLINE_MEM
#define DEBUG_DDR

void parse_soft_settings(unsigned *ptr, unsigned size);
void parse_hard_settings(unsigned *ptr, unsigned size);

void __init prom_setup_cmdline(void);

#ifdef DEBUG_DDR
void cons_puthex4(u32 h){
	h&=0x0f;
	if (h>=10)
		cons_putc((h-10)+'a');
	else
		cons_putc(h+'0');
}

void cons_putreg32(u32 reg){
	char c;
	cons_putc('0');
	cons_putc('x');
	for (c=28;c>=0;c-=4)
		cons_puthex4(reg>>c);
}
#endif

void __init prom_init(void)
{
// 	unsigned i, offset = 0;

	DDR_t ddr = (DDR_t) DDR_VirtualAddress; /* define the pointer to the DDR registers */
	
	/* this should be the very first message, even before serial 
	is properly initialized */
	cons_puts("Linux kernel: Prom init.\n"); 
	prom_setup_cmdline();

#ifdef DEBUG_DDR	
	cons_puts("end cmdline\nRead DDRC DDRbase: ");
	cons_putreg32(ddr->ddrbase);
	cons_puts("\n          DDMASK:  ");
	cons_putreg32(ddr->ddrmask);
	cons_puts("\n          DDRC:    ");
	cons_putreg32(ddr->ddrc);
	cons_puts("\n          DDRAMASK:");
	cons_putreg32(ddr->ddramask); /* should be 0 */
	cons_puts(".\n");
#endif

#ifdef CONFIG_MIKROTIK_RB500
	soft_reboot = read_c0_status() & SR_NMI;
#endif

	setup_serial_port();

	/* set our arch type */
	mips_machgroup = MACH_GROUP_MIKROTIK;

	/*
	 * give all RAM to boot allocator,
	 * except where the kernel was loaded
	 */

	/* Old one:
	add_memory_region(0,rc32434_res_ram.end - rc32434_res_ram.start, BOOT_MEM_RAM);*/
	add_memory_region(ddr->ddrbase,0-ddr->ddrmask,BOOT_MEM_RAM);
	
#ifdef CONFIG_REMOTE_DEBUG
	if(strstr(arcs_cmdline, "kgdb=on")) {
		remote_debug = 1;
	}
#endif
	pm_power_off = NULL; // TODO
	
	/*cons_puts("Prom init end\n");*/
// 	return 0;
}

void prom_free_prom_memory(void)
{
	printk("stubbed prom_free_prom_memory()\n");
}

void __init prom_setup_cmdline(void){
	char cmd_line[CL_SIZE];
	char *cp;
	int prom_argc;
	char **prom_argv, **prom_envp;
	int i;
	
	prom_argc = fw_arg0;
	prom_argv = (char **) fw_arg1;
	prom_envp = (char **) fw_arg2;
	
	cp=cmd_line;
		/* Note: it is common that parameters start at argv[1] and not argv[0],
		however, our elf loader starts at [0] */
	for(i=0;i<prom_argc;i++){
		if (strncmp(prom_argv[i], FREQ_TAG, sizeof(FREQ_TAG) - 1) == 0) {
			idt_cpu_freq = simple_strtoul(prom_argv[i] + sizeof(FREQ_TAG) - 1, 0, 10);
			continue;
		}
#ifdef IGNORE_CMDLINE_MEM
		/* parses out the "mem=xx" arg */
		if (strncmp(prom_argv[i], MEM_TAG, sizeof(MEM_TAG) - 1) == 0) {
			continue;
		}
#endif
		if (i>0) *(cp++) = ' ';
		if (strncmp(prom_argv[i], BOARD_TAG, sizeof(BOARD_TAG) - 1) == 0) {
			board_type =  simple_strtoul(prom_argv[i] + sizeof(BOARD_TAG) - 1, 0, 10);
		}
		if (strncmp(prom_argv[i], GPIO_TAG, sizeof(GPIO_TAG) - 1) == 0) {
			gpio_bootup_state =  simple_strtoul(prom_argv[i] + sizeof(GPIO_TAG) - 1, 0, 10);
		}
		strcpy(cp,prom_argv[i]);
		cp+=strlen(prom_argv[i]);
	}
	
	i=strlen(arcs_cmdline);
	if (i>0){
		*(cp++) = ' ';
		strcpy(cp,arcs_cmdline);
		cp+=strlen(arcs_cmdline);
	}
	if (gpio_bootup_state&0x02)
		strcpy(cp,GPIO_INIT_NOBUTTON);
	else
		strcpy(cp,GPIO_INIT_BUTTON);
	cmd_line[CL_SIZE-1] = '\0';
	
	strcpy(arcs_cmdline,cmd_line);
	/*cons_puts("Cmdline: ");
	cons_puts(arcs_cmdline);
	cons_puts("\n");*/
}
static int __init setup_kmac(char *s)
{
    printk("korina mac = %s\n",s);
    strcpy(mips_mac_address,s);
    return 0;
}

__setup("kmac=", setup_kmac);

/* eof */

