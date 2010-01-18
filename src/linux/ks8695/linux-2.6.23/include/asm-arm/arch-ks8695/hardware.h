/*
 * include/asm-arm/arch-ks8695/hardware.h
 *
 * Copyright (C) 2006 Ben Dooks <ben@simtec.co.uk>
 * Copyright (C) 2006 Simtec Electronics
 *
 * KS8695 - Memory Map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>

/*
 * Physical RAM address.
 */
#define KS8695_SDRAM_PA		0x00000000


/*
 * We map an entire MiB with the System Configuration Registers in even
 * though only 64KiB is needed. This makes it easier for use with the
 * head debug code as the initial MMU setup only deals in L1 sections.
 */
#define KS8695_IO_PA		0x03F00000
#define KS8695_IO_VA		0xF0000000
#define KS8695_IO_SIZE		SZ_1M

#define KS8695_PCIMEM_PA	0x60000000
#define KS8695_PCIMEM_SIZE	SZ_512M

#define KS8695_PCIIO_PA		0x80000000
#define KS8695_PCIIO_SIZE	SZ_64K


/*
 * PCI support
 */
#define pcibios_assign_all_busses()	1

#define PCIBIOS_MIN_IO		0
#define PCIBIOS_MIN_MEM		0

#ifdef CONFIG_ARCH_KS8695_VSOPENRISC 
/* 
 * memory layout for the VScom OpenRISC 
 * 
 * 0x00000000 - 0x03afffff		// SDRAM	(59MB)
 * 0x03b00000 - 0x03f00000 		// Flash	(4MB)
 * ... 512kb free space ..
 * 0x03f80000 - 0x03fbffff		// PCMCIA	(256kb)
 * 0x03fc0000 - 0x03fcffff 		// ExtIO0	(64kb)
 * 0x03fd0000 - 0x03fdffff	 	// ExtIO1	(64kb)
 * 0x03fe0000 - 0x03feffff 		// ExtIO2	(64kb)
 * 0x03ff0000 - 0x04000000		// CfgBase	(64kb)
 *
 * EXTIO0: OpenRISC internal components (IDE, I2C, 1us timer, Super I/O)
 * EXTIO1: 
 * EXTIO2:
 * 
 */

#define VSOPENRISC_SDRAM_BASE	0x00000000
#define VSOPENRISC_SDRAM_SIZE	0x03B00000

#define VSOPENRISC_FLASH_BASE	0x03B00000
#define VSOPENRISC_FLASH_SIZE	0x400000

#define VSOPENRISC_EXTIO0_OFFSET	0xC0000
#define VSOPENRISC_PA_EXTIO0_BASE	(KS8695_IO_PA + VSOPENRISC_EXTIO0_OFFSET)
#define VSOPENRISC_VA_EXTIO0_BASE	(KS8695_IO_VA + VSOPENRISC_EXTIO0_OFFSET)
#define VSOPENRISC_EXTIO0_SIZE		0x10000

#define VSOPENRISC_EXTIO1_OFFSET	0xD0000
#define VSOPENRISC_PA_EXTIO1_BASE	(KS8695_IO_PA + VSOPENRISC_EXTIO1_OFFSET)
#define VSOPENRISC_VA_EXTIO1_BASE	(KS8695_IO_VA + VSOPENRISC_EXTIO1_OFFSET)
#define VSOPENRISC_EXTIO1_SIZE		0x10000

#define VSOPENRISC_EXTIO2_OFFSET	0xE0000
#define VSOPENRISC_PA_EXTIO2_BASE	(KS8695_IO_PA + VSOPENRISC_EXTIO2_OFFSET)
#define VSOPENRISC_VA_EXTIO2_BASE	(KS8695_IO_VA + VSOPENRISC_EXTIO2_OFFSET)
#define VSOPENRISC_EXTIO2_SIZE		0x10000

#endif // CONFIG_ARCH_KS8695_VSOPENRISC

#endif
