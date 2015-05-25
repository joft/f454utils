OPENVPN_VERSION ?= 2.3.6
OPENVPN_SHA256SUM ?= 7baed2ff39c12e1a1a289ec0b46fcc49ff094ca58b8d8d5f29b36ac649ee5b26

OPENVPN_DIR ?= openvpn-$(OPENVPN_VERSION)
OPENVPN_TAR ?= $(OPENVPN_DIR).tar.gz
OPENVPN_URL ?= http://swupdate.openvpn.org/community/releases/$(OPENVPN_TAR)

ifneq ($(CROSS_COMPILE),)
SDK ?= /SDK

SDK_TOP ?= $(SDK)/arago-2011.06
SDK_SYSROOT ?= $(SDK_TOP)/armv5te/arm-none-linux-gnueabi

ifneq ($(dir $(CROSS_COMPILE)),./)
CROSS_COMPILE_PATH = $(dir $(CROSS_COMPILE))
endif
endif

BUILD_DIR = build
BUILD_F454_DIR = $(BUILD_DIR)/f454
EXE_F454_FILE = $(BUILD_F454_DIR)/src/openvpn/openvpn
STRIP_F454_FILE = $(EXE_F454_FILE)_strip


all: $(STRIP_F454_FILE)

.SUFFIXES:

download-stamp:
	wget -O $(OPENVPN_TAR) $(OPENVPN_URL)
	touch $@

$(OPENVPN_TAR).sha256sum: download-stamp
	sha256sum $(OPENVPN_TAR) | cut -d " " -f 1 >$@

check-stamp: $(OPENVPN_TAR).sha256sum
	[ `cat $<` = "$(OPENVPN_SHA256SUM)" ]
	touch $@

unpack-stamp: check-stamp
	tar xf $(OPENVPN_TAR)
	touch $@

patch-stamp: unpack-stamp
	cd $(OPENVPN_DIR) && \
		patch -p1 <../0001-src-openvpn-Add-support-for-being-run-as-a-bticino-c.patch
	cd $(OPENVPN_DIR) && \
		autoreconf -vi
	touch $@

pkg-config.env:
	echo -n "" >$@
ifneq ($(CROSS_COMPILE),)
ifneq ($(CROSS_COMPILE_PATH),)
	echo "export PATH=\$$PATH:$(CROSS_COMPILE_PATH)" >>$@
endif
ifeq ($(PKG_CONFIG_SYSROOT_DIR),)
	echo "export PKG_CONFIG_SYSROOT_DIR=$(SDK_SYSROOT)" >>$@
	echo "export PKG_CONFIG_LIBDIR=$(SDK_SYSROOT)/usr/lib/pkgconfig" >>$@
	echo "export PKG_CONFIG_PATH=\$$PKG_CONFIG_LIBDIR" >>$@
	echo "export PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=" >>$@
	echo "export PKG_CONFIG_ALLOW_SYSTEM_LIBS=" >>$@
endif
endif

configure-stamp: patch-stamp pkg-config.env
	mkdir -p $(BUILD_F454_DIR)
	cd $(BUILD_F454_DIR) && \
		. ../../pkg-config.env && \
		../../$(OPENVPN_DIR)/configure \
			$(if $(CROSS_COMPILE),$(patsubst %-,--host=%,$(shell basename $(CROSS_COMPILE))),) \
			--disable-lzo \
			--disable-plugin-auth-pam \
			--enable-btapp
	touch $@

compile-stamp: configure-stamp
	cd $(BUILD_F454_DIR) && \
		. ../../pkg-config.env && \
		$(MAKE)
	touch $@

$(EXE_F454_FILE): compile-stamp

$(STRIP_F454_FILE): $(EXE_F454_FILE)
	$(CROSS_COMPILE)strip -v -g $< -o $@

################################################################################

# lazy rule, without stamps, to build same openvpn version for the host
BUILD_HOST_DIR = $(BUILD_DIR)/host
EXE_HOST_FILE = $(BUILD_HOST_DIR)/src/openvpn/openvpn

$(EXE_HOST_FILE): patch-stamp
	mkdir -p $(BUILD_HOST_DIR)
	cd $(BUILD_HOST_DIR) && \
		../../$(OPENVPN_DIR)/configure \
			--disable-lzo \
			--disable-plugin-auth-pam && \
		$(MAKE)

################################################################################

CIPHER = AES-256-CBC
AUTH = SHA256

# TLS cipher selection, see https://community.openvpn.net/openvpn/wiki/Hardening
# Limited number, due to apparent length restriction of "tls-cipher" option!
# F454's libssl 1.0.0d supports TLSv1.0 DHE only.
#   => no GCM, no SHA2 (SHA256,384), no ECDHE
TLS_CIPHER = \
	TLS-DHE-RSA-WITH-AES-256-CBC-SHA \
	TLS-DHE-RSA-WITH-CAMELLIA-256-CBC-SHA \
	TLS-DHE-RSA-WITH-3DES-EDE-CBC-SHA \
	TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
	TLS-DHE-RSA-WITH-SEED-CBC-SHA \
	TLS-DHE-RSA-WITH-CAMELLIA-128-CBC-SHA


$(APP_DIR)/openvpn: $(STRIP_F454_FILE)
	mkdir -p $(dir $@)
	cp -a $< $@

$(APP_DIR)/up.sh:
	mkdir -p $(dir $@)
	echo '#!/bin/sh' >$@
	echo '' >>$@
	echo '/sbin/sysctl net.ipv4.ip_forward=1' >>$@
	chmod +x $@

$(APP_DIR)/server.conf: $(OPENVPN_DIR)/sample/sample-config-files/server.conf
	mkdir -p $(dir $@)
	cat $(OPENVPN_DIR)/sample/sample-config-files/server.conf | \
		sed \
			-e 's/^[ \t]*comp-lzo/;comp-lzo/' \
			-e 's#^[ \t]*\(status[ \t]\+\)\(.*\)#\1/tmp/\2#' \
			-e 's/^[ \t]*\(cipher[ \t]\+.*\)/;\1/' \
			>$@
	echo '' >>$@
	echo 'cipher $(CIPHER)' >>$@
	echo 'auth $(AUTH)' >>$@
	echo -n 'tls-cipher ' >>$@
	for I in $(TLS_CIPHER); do \
		if [ -n "$$FD" ]; then \
			echo -n ":$$I" >>$@; \
		else \
			echo -n "$$I" >>$@; \
		fi; \
		FD=1; \
	done
	echo '' >>$@
	echo '' >>$@
	echo '# Uncomment to execute script which enables IP Forwarding on start-up.' >>$@
ifneq ($(EXPOSE_LAN),)
	echo 'script-security 2' >>$@
	echo 'up up.sh' >>$@
else
	echo ';script-security 2' >>$@
	echo ';up up.sh' >>$@
endif
	echo '' >>$@
	echo '# Uncomment and adjust to provide clients with route to LAN.' >>$@
	echo '# Then uncomment above options to enable IP Forwarding, too!' >>$@
ifneq ($(EXPOSE_LAN),)
	echo 'push "route $(EXPOSE_LAN)"' >>$@
else
	echo ';push "route 192.168.1.0 255.255.255.0"' >>$@
endif

$(APP_DIR)/create $(APP_DIR): $(APP_DIR)/openvpn $(APP_DIR)/up.sh $(APP_DIR)/server.conf
.PHONY: $(APP_DIR)/create

$(APP_DIR)/test: $(APP_DIR)/create $(EXE_HOST_FILE)
	cd $(APP_DIR) && \
		sudo ../$(EXE_HOST_FILE) --config server.conf
.PHONY: $(APP_DIR)/test

$(APP_DIR)/clean:
	rm -f $(APP_DIR)/ipp.txt
.PHONY: $(APP_DIR)/clean

################################################################################

clean:
	rm -f *-stamp $(OPENVPN_TAR) $(OPENVPN_TAR).sha256sum pkg-config.env
	rm -fr $(OPENVPN_DIR) $(BUILD_DIR)
.PHONY: clean

almostclean:
	rm -f patch-stamp unpack-stamp configure-stamp compile-stamp \
		pkg-config.env
	rm -fr $(OPENVPN_DIR) $(BUILD_DIR)
.PHONY: almostclean
