# GnuTLS

GNUTLS_VERSION := 3.3.22
GNUTLS_URL := ftp://ftp.gnutls.org/gcrypt/gnutls/v3.3/gnutls-$(GNUTLS_VERSION).tar.xz

ifdef BUILD_NETWORK
ifndef HAVE_DARWIN_OS
PKGS += gnutls
endif
endif
ifeq ($(call need_pkg,"gnutls >= 3.2.0"),)
PKGS_FOUND += gnutls
endif

$(TARBALLS)/gnutls-$(GNUTLS_VERSION).tar.xz:
	$(call download,$(GNUTLS_URL))

.sum-gnutls: gnutls-$(GNUTLS_VERSION).tar.xz

gnutls: gnutls-$(GNUTLS_VERSION).tar.xz .sum-gnutls
	$(UNPACK)
ifdef HAVE_WIN32
	$(APPLY) $(SRC)/gnutls/gnutls-win32.patch
	$(APPLY) $(SRC)/gnutls/gnutls-mingw64.patch
endif
ifdef HAVE_ANDROID
	$(APPLY) $(SRC)/gnutls/no-create-time-h.patch
endif
	$(APPLY) $(SRC)/gnutls/read-file-limits.h.patch
	$(APPLY) $(SRC)/gnutls/mac-keychain-lookup.patch
ifdef HAVE_MACOSX
	$(APPLY) $(SRC)/gnutls/gnutls-pkgconfig-osx.patch
endif
	$(call pkg_static,"lib/gnutls.pc.in")
	$(UPDATE_AUTOCONFIG)
	$(MOVE)

GNUTLS_CONF := \
	--disable-gtk-doc \
	--without-p11-kit \
	--disable-cxx \
	--disable-srp-authentication \
	--disable-psk-authentication-FIXME \
	--disable-anon-authentication \
	--disable-openpgp-authentication \
	--disable-openssl-compatibility \
	--disable-guile \
	--disable-nls \
	--without-libintl-prefix \
	--disable-doc \
	--disable-tests \
	$(HOSTCONF)

GNUTLS_ENV := $(HOSTVARS)

DEPS_gnutls = nettle $(DEPS_nettle)

ifdef HAVE_ANDROID
GNUTLS_ENV += gl_cv_header_working_stdint_h=yes
endif
ifdef HAVE_TIZEN
	GNUTLS_CONF += --with-default-trust-store-dir=/etc/ssl/certs/
endif

.gnutls: gnutls
	$(RECONF)
	cd $< && $(GNUTLS_ENV) ./configure $(GNUTLS_CONF)
	cd $</gl && $(MAKE) install
	cd $</lib && $(MAKE) install
	touch $@
