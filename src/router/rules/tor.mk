miniupnpc:
	make -C miniupnpc OS="Linux" OS_STRING="DD-WRT" PREFIX="/usr" CFLAGS="$(COPTS) $(MIPS16_OPT) -D_GNU_SOURCE -ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC"

miniupnpc-clean:
	make -C miniupnpc clean

miniupnpc-configure:
	make -C miniupnpc

miniupnpc-install:
	make -C miniupnpc install INSTALLPREFIX=$(INSTALLDIR)/miniupnpc/usr 
	rm -rf $(INSTALLDIR)/miniupnpc/usr/bin
	rm -rf $(INSTALLDIR)/miniupnpc/usr/include
	rm -rf $(INSTALLDIR)/miniupnpc/usr/share
	rm -f $(INSTALLDIR)/miniupnpc/usr/lib/*.a

tor-configure: libevent
	cd tor && ./configure  --prefix=/usr ac_cv_host=$(ARCH)-uclibc-linux --target=$(ARCH)-linux --disable-systemd --host=$(ARCH) CC="ccache $(ARCH)-linux-uclibc-gcc" \
	--disable-asciidoc \
	--disable-tool-name-check \
	--disable-gcc-hardening \
	CFLAGS="$(COPTS) $(MIPS16_OPT) -std=gnu99 -ffunction-sections -fdata-sections -Wl,--gc-sections  -I$(TOP)/zlib -I$(TOP) -I$(TOP)/openssl/include -I$(TOP)/libevent -I$(TOP)/libevent/include" \
	CPPFLAGS="$(COPTS) $(MIPS16_OPT) -std=gnu99 -ffunction-sections -fdata-sections -Wl,--gc-sections  -I$(TOP)/zlib -I$(TOP) -I$(TOP)/openssl/include -I$(TOP)/libevent  -I$(TOP)/libevent/include" \
	LDFLAGS="$(COPTS) $(MIPS16_OPT) -std=gnu99  -L$(TOP)/zlib   -L$(TOP)/openssl -L$(TOP)/libevent/.libs" 
	touch tor/*

tor: libevent
	touch tor/*
	make -C tor

tor-clean:
	touch tor/*
	make -C tor clean

tor-install:
	make -C tor DESTDIR=$(INSTALLDIR)/tor install
	mkdir -p $(INSTALLDIR)/tor/etc/config
	install -D tor/config/tor.webservices $(INSTALLDIR)/tor/etc/config
	install -D tor/config/tor.nvramconfig $(INSTALLDIR)/tor/etc/config