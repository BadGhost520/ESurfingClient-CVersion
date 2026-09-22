include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE

PKG_VERSION:=2.1.1
PKG_RELEASE:=5

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=libcurl

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

ifneq ($(filter mips%,$(ARCH)),)
  EXTRA_MIPS_FLAGS := -minterlink-mips16
else
  EXTRA_MIPS_FLAGS :=
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(CURDIR)/app/* $(PKG_BUILD_DIR)/
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
echo "ESurfing 客户端主程序安装完成"

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

exit 0
endef

define Package/esurfingclient/postrm
#!/bin/sh
echo "正在清理 ESurfing 客户端残留文件"

# 会话存档 (<配置文件路径>.<账号序号>.logout): 只有被强杀过才会留下, 卸载后没有意义
rm -f /etc/config/esurfingclient.*.logout

# 升级时留下的模板文件
rm -f /etc/config/esurfingclient.apk-new
rm -f /etc/config/esurfingclient.opkg-new
rm -f /etc/config/esurfingclient.opkg

# 默认日志目录 (配置里自己指定过 log_dir 的路径不动, 免得误删用户自己的东西)
rm -rf /var/log/esurfing

exit 0
endef

define Package/esurfingclient/description
轻量第三方电信校园网认证客户端 (支持 OpenWrt/Linux/Windows 多平台运行)
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/esurfingclient

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/etc/init.d/esurfingclient $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/files/etc/config/esurfingclient $(1)/etc/config/esurfingclient
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

$(eval $(call BuildPackage,esurfingclient))
