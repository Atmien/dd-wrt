#ifndef __MIPS_RB_H__
#define __MIPS_RB_H__

#define IDT434_REG_BASE ((volatile void *) KSEG1ADDR(0x18000000))
#define DEV0BASE 0x010000
#define DEV0MASK 0x010004
#define DEV0C 0x010008
#define DEV0TC 0x01000C
#define DEV1BASE 0x010010
#define DEV1MASK 0x010014
#define DEV1C 0x010018
#define DEV1TC 0x01001C
#define DEV2BASE 0x010020
#define DEV2MASK 0x010024
#define DEV2C 0x010028
#define DEV2TC 0x01002C
#define DEV3BASE 0x010030
#define DEV3MASK 0x010034
#define DEV3C 0x010038
#define DEV3TC 0x01003C
#define BTCS 0x010040
#define BTCOMPARE 0x010044
#define GPIOFUNC 0x050000
#define GPIOCFG 0x050004
#define GPIOD 0x050008
#define GPIOILEVEL 0x05000C
#define GPIOISTAT 0x050010
#define GPIONMIEN 0x050014
#define IMASK6 0x038038

#define LO_WPX (1 << 0)
#define LO_ALE (1 << 1)
#define LO_CLE (1 << 2)
#define LO_CEX (1 << 3)
#define LO_FOFF (1 << 5)
#define LO_SPICS (1 << 6)
#define LO_ULED (1 << 7)

void changeLatchU5(unsigned char orMask, unsigned char nandMask);
unsigned get434Reg(unsigned regOffs);
void set434Reg(unsigned regOffs, unsigned bit, unsigned len, unsigned val);

#define get434Reg(x) (*(volatile unsigned *) (IDT434_REG_BASE + (x)))

#endif
