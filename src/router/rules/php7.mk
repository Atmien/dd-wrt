php7: libxml2 libmcrypt
	CC="ccache $(ARCH)-linux-uclibc-gcc" \
	CFLAGS="$(COPTS) $(MIPS16_OPT)   -I$(TOP)/libgd/libpng -I$(TOP)/libxml2/include -I$(TOP)/curl/include -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	CPPFLAGS="$(COPTS) $(MIPS16_OPT) -I$(TOP)/libgd/libpng -I$(TOP)/libxml2/include -I$(TOP)/curl/include -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	LDFLAGS="$(COPTS) $(MIPS16_OPT) -L$(TOP)/libgd/libpng/.libs -L$(TOP)/libxml2/.libs -lxml2 -L$(TOP)/glib20/libiconv/lib/.libs -liconv -L$(TOP)/zlib -L$(TOP)/openssl -L$(TOP)/zlib -lz -L$(TOP)/curl/lib/.libs -lcurl -lcrypto -lssl -ldl -fPIC -v -Wl,--verbose" \
	$(MAKE) -C php7
	
	
PHP_CONFIGURE_ARGS= \
	--program-prefix= \
	--program-suffix= \
	--prefix=/usr \
	--exec-prefix=/usr \
	--bindir=/usr/bin \
	--datadir=/usr/share \
	--infodir=/usr/share/info \
	--includedir=/ \
	--oldincludedir=/ \
	--libdir=/usr/lib \
	--libexecdir=/usr/lib \
	--localstatedir=/var \
	--mandir=/usr/share/man \
	--sbindir=/usr/sbin \
	--sysconfdir=/etc \
	--with-config-file-scan-dir=/jffs/etc \
	--with-iconv-dir="$(TOP)/glib20/libiconv" \
	--enable-shared \
	--enable-static \
	--disable-rpath \
	--disable-debug \
	--without-pear \
	--with-libxml-dir="$(TOP)/libxml2" \
	--with-config-file-path=/etc \
	--disable-short-tags \
	--disable-ftp \
	--without-gettext \
	--disable-mbregex \
	--with-openssl-dir="$(TOP)/openssl" \
	--with-openssl=shared,"$(TOP)/openssl" \
	--with-kerberos=no \
	--disable-phar \
	--with-kerberos=no \
	--disable-soap \
	--enable-sockets \
	--disable-tokenizer \
	--without-freetype-dir \
	--without-xpm-dir \
	--without-t1lib \
	--disable-gd-jis-conv \
	--enable-cli \
	--enable-cgi \
	--enable-zip \
	--enable-mbstring \
	--enable-maintainer-zts \
	--with-tsrm-pthreads \
	--with-gd \
	--with-zlib \
	--with-zlib-dir="$(TOP)/zlib" \
	--with-png-dir="$(TOP)/libgd/libpng/.libs" \
	--with-jpeg-dir="$(TOP)/minidlna/jpeg-8" \
	--with-mcrypt="$(TOP)/libmcrypt" \
	--with-curl="$(TOP)/curl" \
	php_cv_cc_rpath="no" \
	iconv_impl_name="gnu_libiconv" \
	ac_cv_lib_png_png_write_image="yes" \
	ac_cv_lib_crypt_crypt="yes" \
	ac_cv_lib_z_gzgets="yes" \
	ac_cv_php_xml2_config_path="$(TOP)/libxml2/xml2-config" \
	ac_cv_lib_z_gzgets="yes" \
	ac_cv_lib_crypto_X509_free="yes" \
	ac_cv_lib_ssl_DSA_get_default_method="yes" \
	ac_cv_func_crypt="yes" \
	ac_cv_lib_crypto_CRYPTO_free="yes" \
	ac_cv_header_openssl_crypto_h="yes" \
	ac_cv_lib_ssl_SSL_CTX_set_ssl_version="yes" \
	ac_cv_glob="yes" \
	ICONV_DIR="$(TOP)/glib20/libiconv" \
	OPENSSL_LIBDIR="$(TOP)/openssl" \
	PHP_OPENSSL_DIR="$(TOP)/openssl" \
	PHP_CURL="$(TOP)/curl" \
	PHP_ICONV="$(TOP)/glib20/libiconv" \
	EXTRA_CFLAGS="-L$(TOP)/glib20/libiconv/lib/.libs -liconv -I$(TOP)/libmcrypt -I$(TOP)/zlib -I$(TOP)/libgd/libpng -lcrypt -L$(TOP)/openssl -I$(TOP)/curl/include -lcrypto -lssl" \
	EXTRA_LIBS="-liconv " \
	EXTRA_LDFLAGS="-L$(TOP)/libmcrypt/lib/.libs -lmcrypt -L$(TOP)/glib20/libiconv/lib/.libs -liconv -L$(TOP)/libxml2/.libs -lxml2 -L$(TOP)/zlib -L$(TOP)/libgd/libpng/.libs -lpng -L$(TOP)/libgd/src/.libs -lgd -L$(TOP)/openssl -L$(TOP)/zlib -lz -L$(TOP)/curl/lib/.libs -lcurl -lcrypto -lssl -lcrypt -ldl" \
	EXTRA_LDFLAGS_PROGRAM="-L$(TOP)/libmcrypt/lib/.libs -lmcrypt -L$(TOP)/glib20/libiconv/lib/.libs -liconv -L$(TOP)/libxml2/.libs -lxml2 -L$(TOP)/libgd/libpng/.libs -lpng -L$(TOP)/libgd/src/.libs -lgd -L$(TOP)/openssl -L$(TOP)/zlib -lz -L$(TOP)/curl/lib/.libs -lcurl -lcrypto -lssl -lcrypt -ldl"

ifeq ($(ARCH),mips64)
PHP_ENDIAN=ac_cv_c_bigendian_php="yes"
endif
ifeq ($(ARCH),mips)
PHP_ENDIAN=ac_cv_c_bigendian_php="yes"
endif
ifeq ($(ARCH),armeb)
PHP_ENDIAN=ac_cv_c_bigendian_php="yes"
endif
ifeq ($(ARCH),powerpc)
PHP_ENDIAN=ac_cv_c_bigendian_php="yes"
endif

	
php7-configure: minidlna-configure libpng-configure libgd-configure libxml2-configure libpng libgd libxml2 zlib curl
	rm -f php7/config.cache
	rm -rf php7/autom4te.cache
	cd php7 && autoconf
	cd php7 && './configure'  '--host=$(ARCH)-linux-uclibc' $(PHP_ENDIAN) $(PHP_CONFIGURE_ARGS) \
	'CFLAGS=$(COPTS) -I$(TOP)/minidlna/jpeg-8 -I$(TOP)/libmcrypt -I$(TOP)/libgd/libpng -I$(TOP)/libxml2/include -I$(TOP)/glib20/libiconv/include -I$(TOP)/curl/include -DNEED_PRINTF -L$(TOP)/glib20/libiconv/lib/.libs -L$(TOP)/zlib -lz -L$(TOP)/curl/lib/.libs -lcurl -liconv' \
	'LDFLAGS=-L$(TOP)/minidlna/lib -ljpeg -L$(TOP)/libmcrypt/lib/.libs -lmcrypt -L$(TOP)/libxml2/.libs -L$(TOP)/zlib -L$(TOP)/libgd/libpng/.libs -lpng -L$(TOP)/libgd/src/.libs -lgd -L$(TOP)/glib20/libiconv/lib/.libs -liconv -L$(TOP)/openssl -L$(TOP)/zlib -lz -L$(TOP)/curl/lib/.libs -lcurl -lcrypto -lssl -lcrypt -ldl'
	printf "#define HAVE_GLOB 1\n" >>$(TOP)/php7/main/php_config.h

php7-clean:
	if test -e "php7/Makefile"; then make -C php7 clean; fi

php7-install:
ifeq ($(CONFIG_PHPCMD),y)
	install -D php7/sapi/cli/.libs/php $(INSTALLDIR)/php7/usr/bin/php
endif
ifneq ($(CONFIG_PHPCGI),y)
	install -D php7/sapi/cli/.libs/php $(INSTALLDIR)/php7/usr/bin/php
endif
ifeq ($(CONFIG_PHPCGI),y)
	install -D php7/sapi/cgi/.libs/php-cgi $(INSTALLDIR)/php7/usr/bin/php-cgi
	mkdir -p $(INSTALLDIR)/php7/etc/php/modules
	cp php7/modules/*.so $(INSTALLDIR)/php7/etc/php/modules
	printf "short_open_tag=on\ncgi.fix_pathinfo=1\n" >$(INSTALLDIR)/php7/etc/php.ini
	printf "post_max_size = 32M\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "upload_max_filesize = 32M\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "output_buffering = Off\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "extension_dir = /etc/php/modules\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "extension = openssl.so\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "zend_extension = opcache.so\n" >>$(INSTALLDIR)/php7/etc/php.ini
endif
ifeq ($(CONFIG_LIGHTTPD),y)
	install -D php7/sapi/cgi/.libs/php-cgi $(INSTALLDIR)/php7/usr/bin/php-cgi
	mkdir -p $(INSTALLDIR)/php7/etc/php/modules
	cp php7/modules/*.so $(INSTALLDIR)/php7/etc/php/modules
	printf "short_open_tag=on\ncgi.fix_pathinfo=1\n" >$(INSTALLDIR)/php7/etc/php.ini
	printf "post_max_size = 32M\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "upload_max_filesize = 32M\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "output_buffering = Off\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "extension_dir = /etc/php/modules\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "extension = openssl.so\n" >>$(INSTALLDIR)/php7/etc/php.ini
	printf "zend_extension = opcache.so\n" >>$(INSTALLDIR)/php7/etc/php.ini
	
endif
