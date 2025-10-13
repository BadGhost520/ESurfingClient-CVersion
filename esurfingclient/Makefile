include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=esurfingclient-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=file://$(TOPDIR)/dl
PKG_HASH:=skip
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=curl openssl
DEPENDS:=+libcurl +libopenssl +ca-bundle

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

# 若压缩包顶层是子目录（如 test1），取消注释并设置
CMAKE_SOURCE_SUBDIR:=esurfing

CMAKE_INSTALL:=1
CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_EXE_LINKER_FLAGS="-s"

define Package/esurfingclient
  SECTION:=net
  CATEGORY:=Network
  TITLE:=ESurfingClient
  DEPENDS:=+libcurl +libopenssl +ca-bundle
endef

define Package/esurfingclient/description
A lightweight client using libcurl and OpenSSL.
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/

	# 安装 init 脚本与 UCI 配置（放在包的 files/ 目录）
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/esurfingclient.config $(1)/etc/config/esurfingclient
endef

$(eval $(call BuildPackage,esurfingclient))
