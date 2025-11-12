include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_VERSION:=1.0.3
PKG_RELEASE:=1

PKG_SOURCE:=esurfingclient.tar.gz
PKG_SOURCE_URL:=file://$(TOPDIR)/dl
PKG_SOURCE_SUBDIR:=esurfing
PKG_BUILD_DIR:=$(BUILD_DIR)/esurfing
PKG_HASH:=skip
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=curl openssl
DEPENDS:=+libcurl +libopenssl +ca-bundle

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_SOURCE_SUBDIR:=

CMAKE_INSTALL:=1
CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_SHARED_LIBS=OFF

define Package/esurfingclient
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ESurfingClient
	DEPENDS:=+libcurl +libopenssl +ca-bundle
endef

define Package/esurfingclient/description
A lightweight esurfing authentication client
endef

define Package/esurfingclient/conffiles
/etc/config/esurfingclient
endef

define Build/Compile
	$(call Build/Compile/Default)

	$(TARGET_CROSS)strip --strip-all $(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/esurfingclient.config $(1)/etc/config/esurfingclient
endef

$(eval $(call BuildPackage,esurfingclient))