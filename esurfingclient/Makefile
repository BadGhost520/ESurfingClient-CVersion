include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=libcurl libopenssl

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
	-DCMAKE_EXE_LINKER_FLAGS="-static -lpthread -latomic -mno-mips16" \
	-DCMAKE_C_FLAGS="-DCURL_STATICLIB -pthread -D__OPENWRT__=1 -mno-mips16"

define Package/esurfingclient
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ESurfingClient
endef

define Package/esurfingclient/prerm
#!/bin/sh
if [ -f "/usr/bin/ESurfingClient.json" ]; then
    cp /usr/bin/ESurfingClient.json /usr/bin/ESurfingClient.json.bak
    echo "检测到旧配置文件, 已备份到 /usr/bin/ESurfingClient.json.bak"
    echo "使用 cp /usr/bin/ESurfingClient.json.bak /usr/bin/ESurfingClient.json 恢复"
fi
exit 0
endef

define Package/esurfingclient/description
轻量第三方电信校园网客户端
支持多平台运行
Powerby: BadGhost
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/etc/init.d/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/files/etc/config/esurfingclient.config $(1)/etc/config/esurfingclient

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(CURDIR)/files/usr/bin/ESurfingClient.json $(1)/usr/bin/
endef

$(eval $(call BuildPackage,esurfingclient))
