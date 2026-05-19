include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=../LICENSE

PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=libcurl libopenssl

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

$(warning DEBUG: ARCH is $(ARCH))

ifneq ($(filter mips%,$(ARCH)),)
  EXTRA_MIPS_FLAGS := -minterlink-mips16
else
  EXTRA_MIPS_FLAGS :=
endif

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
	-DCMAKE_EXE_LINKER_FLAGS="-static -lpthread -latomic" \
	-DCMAKE_C_FLAGS="-DCURL_STATICLIB -pthread $(EXTRA_MIPS_FLAGS)" \
	-DCMAKE_CXX_FLAGS="-DCURL_STATICLIB -pthread $(EXTRA_MIPS_FLAGS)" \
	-D__OPENWRT__=1

define Package/esurfingclient
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ESurfingClient
	PKGARCH:=all
endef

define Package/esurfingclient/conffiles
/etc/config/esurfingclient
endef

define Package/esurfingclient/postinst
#!/bin/sh
echo "ESurfing 客户端安装完成"

if [ -f "/etc/config/esurfingclient.apk-new" ]; then
	echo "有新的模板文件, 可查看是否有更新"
	echo "-> /etc/config/esurfingclient.apk-new"
fi

if [ -f "/etc/config/esurfingclient.opkg-new" ]; then
	echo "有新的模板文件, 可查看是否有更新"
	echo "-> /etc/config/esurfingclient.opkg-new"
fi

if [ -f "/etc/config/esurfingclient.opkg" ]; then
	echo "有新的模板文件, 可查看是否有更新"
	echo "-> /etc/config/esurfingclient.opkg"
fi

if [ -f "/etc/init.d/esurfingclient.apk-new" ]; then
	mv /etc/init.d/esurfingclient.apk-new /etc/init.d/esurfingclient
fi

if [ -x "/etc/init.d/esurfingclient" ]; then
	echo "重启服务中"
    /etc/init.d/esurfingclient restart 2>/dev/null &
fi
exit 0
endef

define Package/esurfingclient/description
轻量第三方电信校园网认证客户端 (支持 OpenWrt/Linux/Windows 多平台运行)
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/esurfingclient

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/init.d/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/config/ESurfingClient.json $(1)/etc/config/esurfingclient
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

$(eval $(call BuildPackage,esurfingclient))