include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=libcurl libopenssl libnghttp2

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(CURDIR)/esurfing/* $(PKG_BUILD_DIR)/
endef

CMAKE_SOURCE_SUBDIR:=.
CMAKE_INSTALL:=1
CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_EXE_LINKER_FLAGS="-lnghttp2 -lpthread -latomic" \
	-DCMAKE_C_FLAGS="-DCURL_STATICLIB" \
	-DCMAKE_THREAD_LIBS_INIT="-lpthread" \
	-DCMAKE_USE_PTHREADS_INIT=ON

define Package/esurfingclient
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ESurfingClient
	DEPENDS:=+libcurl +libopenssl +ca-bundle +libnghttp2
endef

define Package/esurfingclient/description
A lightweight esurfing authentication client
endef

define Build/Compile
	$(call Build/Compile/Default)
	$(TARGET_CROSS)strip --strip-all $(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/esurfingclient
	$(CP) $(PKG_BUILD_DIR)/ESurfingClient $(1)/usr/esurfingclient/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/etc/init.d/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(CURDIR)/files/usr/bin/dialer.json $(1)/usr/bin/
endef

$(eval $(call BuildPackage,esurfingclient))