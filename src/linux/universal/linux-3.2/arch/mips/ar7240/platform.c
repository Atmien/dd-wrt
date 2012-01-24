#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>

#include <linux/console.h>
#include <asm/serial.h>

#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <asm/mach-ar7240/ar7240.h>
#include <asm/mach-ar71xx/ar71xx.h>
#include "nvram.h"
#include <asm/mach-ar71xx/ar933x_uart_platform.h>

void serial_print(char *fmt, ...);

#ifdef CONFIG_WASP_SUPPORT
extern uint32_t ath_ref_clk_freq;
#else
extern uint32_t ar7240_ahb_freq;
#endif

/* 
 * OHCI (USB full speed host controller) 
 */
static struct resource ar7240_usb_ohci_resources[] = {
	[0] = {
		.start		= AR7240_USB_OHCI_BASE,
		.end		= AR7240_USB_OHCI_BASE + AR7240_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7240_CPU_IRQ_USB,
        .end	    = AR7240_CPU_IRQ_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

/* 
 * The dmamask must be set for OHCI to work 
 */
static u64 ohci_dmamask = ~(u32)0;
static struct platform_device ar7240_usb_ohci_device = {
	.name		= "ar7240-ohci",
	.id		    = 0,
	.dev = {
		.dma_mask		= &ohci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7240_usb_ohci_resources),
	.resource	= ar7240_usb_ohci_resources,
};

/* 
 * EHCI (USB full speed host controller) 
 */
static struct resource ar7240_usb_ehci_resources[] = {
	[0] = {
		.start		= AR7240_USB_EHCI_BASE,
		.end		= AR7240_USB_EHCI_BASE + AR7240_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7240_CPU_IRQ_USB,
		.end		= AR7240_CPU_IRQ_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

/* 
 * The dmamask must be set for EHCI to work 
 */
static u64 ehci_dmamask = ~(u32)0;

static struct platform_device ar7240_usb_ehci_device = {
	.name		= "ar71xx-ehci",
	.id		    = 0,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7240_usb_ehci_resources),
	.resource	= ar7240_usb_ehci_resources,
};



static struct resource ar7240_uart_resources[] = {
	{
		.start = AR7240_UART_BASE,
		.end = AR7240_UART_BASE+0x0fff,
		.flags = IORESOURCE_MEM,
	},
};

#define AR71XX_UART_FLAGS (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_IOREMAP)

static struct plat_serial8250_port ar7240_uart_data[] = {
	{
                .mapbase        = AR7240_UART_BASE,
                .irq            = AR7240_MISC_IRQ_UART,
                .flags          = AR71XX_UART_FLAGS,
                .iotype         = UPIO_MEM32,
                .regshift       = 2,
                .uartclk        = 0, /* ar7240_ahb_freq, */
	},
        { },
};

static struct platform_device ar7240_uart = {
	 .name               = "serial8250",
        .id                 = 0,
        .dev.platform_data  = ar7240_uart_data,
        .num_resources      = 1, 
        .resource           = ar7240_uart_resources

};


static struct resource ar933x_uart_resources[] = {
	{
		.start  = AR933X_UART_BASE,
		.end    = AR933X_UART_BASE + AR71XX_UART_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = AR7240_MISC_IRQ_UART,
		.end    = AR7240_MISC_IRQ_UART,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct ar933x_uart_platform_data ar933x_uart_data;
static struct platform_device ar933x_uart_device = {
	.name           = "ar933x-uart",
	.id             = -1,
	.resource       = ar933x_uart_resources,
	.num_resources  = ARRAY_SIZE(ar933x_uart_resources),
	.dev = {
		.platform_data  = &ar933x_uart_data,
	},
};

static struct resource ath_uart_resources[] = {
	{
	 .start = AR933X_UART_BASE,
	 .end = AR933X_UART_BASE + 0x0fff,
	 .flags = IORESOURCE_MEM,
	 },
};

static struct plat_serial8250_port ath_uart_data[] = {
	{
	 .mapbase = (u32) KSEG1ADDR(AR933X_UART_BASE),
	 .membase = (void __iomem *)((u32) (KSEG1ADDR(AR933X_UART_BASE))),
	 .irq = AR7240_MISC_IRQ_UART,
	 .flags = (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST),
	 .iotype = UPIO_MEM32,
	 .regshift = 2,
	 .uartclk = 0,		/* ath_ahb_freq, */
	 },
	{},
};

static struct platform_device ath_uart = {
	.name = "serial8250",
	.id = 0,
	.dev.platform_data = ath_uart_data,
	.num_resources = 1,
	.resource = ath_uart_resources
};



static struct platform_device *ar7241_platform_devices[] __initdata = {
	&ar7240_usb_ehci_device
};

static struct platform_device *ar7240_platform_devices[] __initdata = {
	&ar7240_usb_ohci_device
};

static struct platform_device *ar724x_platform_devices[] __initdata = {
#ifdef CONFIG_MACH_HORNET
	&ar933x_uart_device,
	&ath_uart
#else
	&ar7240_uart
#endif
};

extern __init ap91_pci_init(u8 *cal_data, u8 *mac_addr);
void ar9xxx_add_device_wmac(u8 *cal_data, u8 *mac_addr) __init;

static void *getCalData(int slot)
{
u8 *base;
for (base=(u8 *) KSEG1ADDR(0x1f000000);base<KSEG1ADDR (0x1ffff000);base+=0x1000) {
	u32 *cal = (u32 *)base;
	if (*cal==0xa55a0000 || *cal==0x5aa50000) { //protection bit is always zero on inflash devices, so we can use for match it
		if (slot) {
			base+=0x4000;
		}
		printk(KERN_INFO "found calibration data for slot %d on 0x%08X\n",slot,base);
		return base;
	}
    }
return NULL;
}

enum ar71xx_soc_type ar71xx_soc;
EXPORT_SYMBOL_GPL(ar71xx_soc);

int __init ar7240_platform_init(void)
{
	int ret;
	void *ee;
#ifdef CONFIG_WR741
	u8 *mac = (u8 *) KSEG1ADDR(0x1f01fc00);
#else
	u8 *mac = NULL;
#endif

#if defined(CONFIG_AR7242_RTL8309G_PHY) || defined(CONFIG_DIR615E)
#ifdef CONFIG_DIR615E
	const char *config = (char *) KSEG1ADDR(0x1f030000);
#else
	const char *config = (char *) KSEG1ADDR(0x1f040000);
#endif
	u8 wlan_mac[6];
	if (nvram_parse_mac_addr(config, 0x10000,"lan_mac=", wlan_mac) == 0) {
		mac = wlan_mac;
	}
#endif



        /* need to set clock appropriately */
#ifdef CONFIG_MACH_HORNET
//	ath_uart_data[0].uartclk = ar7240_ahb_freq;
//	ar933x_uart_data.uartclk = ar7240_ahb_freq;

	ath_uart_data[0].uartclk = ar71xx_ref_freq;
	ar933x_uart_data.uartclk = ar71xx_ref_freq;
#endif

#ifdef CONFIG_WASP_SUPPORT
	ar7240_uart_data[0].uartclk = ath_ref_clk_freq;
#else
	ar7240_uart_data[0].uartclk = ar7240_ahb_freq;
#endif
	ret = platform_add_devices(ar724x_platform_devices, 
                                ARRAY_SIZE(ar724x_platform_devices));

        if (ret < 0)
		return ret; 

	if (is_ar7241() || is_ar7242()  || is_ar933x() || is_wasp()) {
	    ret = platform_add_devices(ar7241_platform_devices, 
                                ARRAY_SIZE(ar7241_platform_devices));
        }
        if (is_ar7240()) {
	    ret = platform_add_devices(ar7240_platform_devices, 
                                ARRAY_SIZE(ar7240_platform_devices));
        }
        
#ifdef CONFIG_MACH_HORNET
	ee = (u8 *) KSEG1ADDR(0x1fff1000);
	ar9xxx_add_device_wmac(ee, NULL);
#else
	ee = getCalData(0);
	ap91_pci_init(ee, mac);
#endif
return ret;
}

arch_initcall(ar7240_platform_init);
    
