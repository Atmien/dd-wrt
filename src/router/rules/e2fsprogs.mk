e2fsprogs-configure:
	cd e2fsprogs && ./configure --host=$(ARCH)-linux CC="$(CC) $(COPTS) $(MIPS16_OPT) -ffunction-sections -fdata-sections -Wl,--gc-sections -D_GNU_SOURCE  -DNEED_PRINTF -std=gnu89" --disable-static --enable-shared --with-gnu-ld --disable-rpath --enable-elf-shlibs --enable-compression --enable-htree --enable-symlink-install --disable-tls --libdir=/lib root_prefix=$(INSTALLDIR)/e2fsprogs
	make -C e2fsprogs

e2fsprogs:
	make -C e2fsprogs

e2fsprogs-clean:
	make -C e2fsprogs clean

e2fsprogs-install:
	-make -C e2fsprogs install DESTDIR=$(INSTALLDIR)/e2fsprogs

ifneq ($(CONFIG_E2FSPROGS_ADV),y)
	-rm -f $(INSTALLDIR)/e2fsprogs/sbin/*fsck*
	rm -rf $(INSTALLDIR)/e2fsprogs/usr/bin
	rm -rf $(INSTALLDIR)/e2fsprogs/usr/sbin
	rm -rf $(INSTALLDIR)/e2fsprogs/usr/lib
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/debugfs
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/dumpe2fs
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/e2image
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/badblocks
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/blkid
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/logsave
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/resize2fs
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/tune2fs
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/e2undo
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/e2label
	rm -f $(INSTALLDIR)/e2fsprogs/sbin/findfs
endif
	rm -rf $(INSTALLDIR)/e2fsprogs/usr/share
	rm -rf $(INSTALLDIR)/e2fsprogs/usr/man
	-cd $(INSTALLDIR)/e2fsprogs/etc && ln -sf /proc/mounts mtab
	true
