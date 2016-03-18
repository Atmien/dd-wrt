#!/bin/sh
OLDPATH=$PATH
DATE=$(date +%m-%d-%Y)
DATE+="-r"
DATE+=$(svnversion -n pb42/src/router/httpd)
export PATH=/xfs/toolchains/toolchain-mips_34kc_gcc-5.3.0_musl-1.1.14/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_r2_gcc-4.7-linaro_uClibc-0.9.33.2/bin:$OLDPATH
#export PATH=/xfs/toolchains/toolchain-mips_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$OLDPATH
#export PATH=/xfs/toolchains/staging_dir_mips_pb42/bin:$OLDPATH
cd pb42/src/router
[ -n "$DO_UPDATE" ] && svn update
cd opt/etc/config
[ -n "$DO_UPDATE" ] && svn update
cd ../../../

#cp .config_wp546_onnet .config
cat .config_wp546 | grep -v "^CONFIG_ROUTERSTYLE\|^CONFIG_ELEGANT\|^CONFIG_BLUE\|^CONFIG_YELLOW\|^CONFIG_CYAN\|^CONFIG_RED\|^CONFIG_GREEN\|^CONFIG_WIKAR\|^CONFIG_KROMO\|^CONFIG_XIRIAN\|^CONFIG_BRAINSLAYER" > .config
echo "CONFIG_BRANDING=y" >> .config
echo "CONFIG_ONNET=y"	>> .config

make -f Makefile.pb42 kernel clean all install
mkdir -p ~/GruppenLW/releases/CUSTOMER/$DATE/onnet/compex_wp546
cd ../../../
cp pb42/src/router/mips-uclibc/lsx-firmware.bin ~/GruppenLW/releases/CUSTOMER/$DATE/onnet/compex_wp546/wp546-firmware.bin
cp pb42/src/router/mips-uclibc/wp546.img ~/GruppenLW/releases/CUSTOMER/$DATE/onnet/compex_wp546/wp546-image.tftp
