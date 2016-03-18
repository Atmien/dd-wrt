#!/bin/sh
OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n pb42/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mips_34kc_gcc-5.3.0_musl-1.1.14/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_r2_gcc-4.8-linaro_uClibc-0.9.33.2/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_gcc-4.1.2-uClibc-0.9.30.1/usr/bin:$OLDPATH
#export PATH=/xfs/toolchains/staging_dir_mips_pb42/bin:$OLDPATH
cd pb42/src/router
[ -n "$DO_UPDATE" ] && svn update
cd opt/etc/config
[ -n "$DO_UPDATE" ] && svn update
cd ../../../
cp .config_ubntm_eneo .config
make -f Makefile.pb42 kernel clean all install
#mkdir -p ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/pbnt_bullet_m5_hp
#mkdir -p ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/ubnt_rocket_m5
#mkdir -p ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/ubnt_NanoStation_m5
mkdir -p ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/ubnt_Loco_M5
cd ../../../

#cp pb42/src/router/mips-uclibc/ar7420-firmware.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_bullet_m5_hp/bs5m-webflash-firmware.bin
#cp pb42/src/router/mips-uclibc/ubntm-dd-wrt.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_bullet_m5_hp/BS5M-DD-WRT.bin

#cp pb42/src/router/mips-uclibc/ar7420-firmware.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_rocket_m5/r5m-webflash-firmware.bin
#cp pb42/src/router/mips-uclibc/ubntm-dd-wrt.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_rocket_m5/R5M-DD-WRT.bin

#cp pb42/src/router/mips-uclibc/ar7420-firmware.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_NanoStation_m5/ns5m-webflash-firmware.bin
#cp pb42/src/router/mips-uclibc/ubntm-dd-wrt.bin ~/GruppenLW/releases/CUSTOMER/$DATE/ipr/ubnt_NanoStation_m5/NS5M-DD-WRT.bin

cp pb42/src/router/mips-uclibc/ar7420-firmware.bin ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/ubnt_Loco_M5/lc5m-webflash-firmware.bin
cp pb42/src/router/mips-uclibc/ubntm-dd-wrt.bin ~/GruppenLW/releases/CUSTOMER/$DATE/eneo/ubnt_Loco_M5/LC5M-DD-WRT.bin
