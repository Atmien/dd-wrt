/*
 *  AR71xx SoC routines
 *
 *  Copyright (C) 2007 Atheros 
 *  Copyright (C) 2007 Sebastian Gottschall <sebastian.gottschall@newmedia-net.de>
 *  some portions:
 *  Copyright (C) 2008 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */
#include <linux/autoconf.h>
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

#include <asm/mach-ar7100/ar7100.h>
#include <linux/rtl8366rb_smi.h>

extern uint32_t ar71xx_ahb_freq;

/* 
 * OHCI (USB full speed host controller) 
 */
static struct resource ar7100_usb_ohci_resources[] = {
	[0] = {
		.start		= AR7100_USB_OHCI_BASE,
		.end		= AR7100_USB_OHCI_BASE + AR7100_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7100_MISC_IRQ_USB_OHCI,
		.end		= AR7100_MISC_IRQ_USB_OHCI,
		.flags		= IORESOURCE_IRQ,
	},
};

/* 
 * The dmamask must be set for OHCI to work 
 */
static u64 ohci_dmamask = ~(u32)0;

static struct platform_device ar7100_usb_ohci_device = {
	.name		= "ar71xx-ohci",
	.id		    = 0,
	.dev = {
		.dma_mask		= &ohci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7100_usb_ohci_resources),
	.resource	= ar7100_usb_ohci_resources,
};

/* 
 * EHCI (USB full speed host controller) 
 */
static struct resource ar7100_usb_ehci_resources[] = {
	[0] = {
		.start		= AR7100_USB_EHCI_BASE,
		.end		= AR7100_USB_EHCI_BASE + AR7100_USB_WINDOW - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= AR7100_CPU_IRQ_USB,
		.end		= AR7100_CPU_IRQ_USB,
		.flags		= IORESOURCE_IRQ,
	},
};

/* 
 * The dmamask must be set for EHCI to work 
 */
static u64 ehci_dmamask = ~(u32)0;

static struct platform_device ar7100_usb_ehci_device = {
	.name		= "ar71xx-ehci",
	.id		    = 0,
	.dev = {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(ar7100_usb_ehci_resources),
	.resource	= ar7100_usb_ehci_resources,
};

static struct resource ar7100_uart_resources[] = {
	{
		.start = AR7100_UART_BASE,
		.end = AR7100_UART_BASE+0x0fff,
		.flags = IORESOURCE_MEM,
	},
};

#define AR71XX_UART_FLAGS (UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_IOREMAP)

static struct plat_serial8250_port ar7100_uart_data[] = {
	{
                .membase        = (char *)KSEG1ADDR(AR7100_UART_BASE),
                .irq            = AR7100_MISC_IRQ_UART,
                .flags          = AR71XX_UART_FLAGS,
                .iotype         = UPIO_MEM32,
                .regshift       = 2,
                .uartclk        = 0, /* ar7100_ahb_freq, */
	},
        { },
};

static struct platform_device ar7100_uart = {
	 .name               = "serial8250",
        .id                 = 0,
        .dev.platform_data  = ar7100_uart_data,
        .num_resources      = 1, 
        .resource           = ar7100_uart_resources

};

#define TL_WR1043ND_GPIO_RTL8366_SDA    18
#define TL_WR1043ND_GPIO_RTL8366_SCK    19


static struct rtl8366rb_smi_platform_data tl_wr1043nd_rtl8366_smi_data = {
	.gpio_sda        = TL_WR1043ND_GPIO_RTL8366_SDA,
	.gpio_sck        = TL_WR1043ND_GPIO_RTL8366_SCK,
};

static struct platform_device tl_wr1043nd_rtl8366_smi_device = {
	.name		= "rtl8366rb-smi",
	.id		= -1,
	.dev = {
		.platform_data	= &tl_wr1043nd_rtl8366_smi_data,
	}
};



static struct platform_device *ar7100_platform_devices[] __initdata = {
	&ar7100_usb_ohci_device,
	&ar7100_usb_ehci_device,
	&ar7100_uart

};



#define AR71XX_USB_RESET_MASK \
	(RESET_MODULE_USB_HOST | RESET_MODULE_USB_PHY \
	| RESET_MODULE_USB_OHCI_DLL)




int __init ar7100_platform_init(void)
{
        /* need to set clock appropriately */
        ar7100_uart_data[0].uartclk = ar71xx_ahb_freq; 

	platform_add_devices(ar7100_platform_devices,ARRAY_SIZE(ar7100_platform_devices));

#ifdef CONFIG_RTL8366RB_SMI 
	platform_device_register(&tl_wr1043nd_rtl8366_smi_device);
#endif
#ifdef CONFIG_RTL8366RB_SMI_MODULE
	platform_device_register(&tl_wr1043nd_rtl8366_smi_device);
#endif
return 0;
}

arch_initcall(ar7100_platform_init);
