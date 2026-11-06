include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE

PKG_VERSION:=1.0.0
PKG_RELEASE:=1

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
endef

define Package/esurfingclient/conffiles
/etc/config/esurfingclient
/etc/esurfingclient/ESurfingClient.json
endef

define Package/esurfingclient/postinst
#!/bin/sh
echo "ESurfing 客户端主程序安装完成"

mkdir -p /etc/esurfingclient

if [ -f "/etc/config/esurfingclient" ] && [ "$$(head -c 1 /etc/config/esurfingclient 2>/dev/null)" = "{" ]; then
	cp "/etc/config/esurfingclient" "/etc/esurfingclient/ESurfingClient.json"
	echo "旧配置已迁移到 /etc/esurfingclient/ESurfingClient.json"
fi

cfg_dir="$$(uci -q get esurfingclient.config.dir 2>/dev/null)"
[ -n "$$cfg_dir" ] || cfg_dir='/etc/esurfingclient'

if [ -f "/etc/config/esurfingclient.apk-new" ]; then
	mv -f "/etc/config/esurfingclient.apk-new" "$$cfg_dir/esurfingclient.apk-new"
	echo "检测到新配置模板, 已移动到 $$cfg_dir/esurfingclient.apk-new (当前配置未改动)"
elif [ -f "/etc/config/esurfingclient-opkg" ]; then
	mv -f "/etc/config/esurfingclient-opkg" "$$cfg_dir/esurfingclient-opkg"
	echo "检测到新配置模板, 已移动到 $$cfg_dir/esurfingclient-opkg (当前配置未改动)"
fi

if [ ! -f "/etc/config/esurfingclient" ] || [ "$$(head -c 1 /etc/config/esurfingclient 2>/dev/null)" = "{" ]; then
	printf "config global 'config'\n\toption dir '/etc/esurfingclient'\n" > /etc/config/esurfingclient
fi

changed=0

uci -q get esurfingclient.config.dir >/dev/null 2>&1 || {
	uci set esurfingclient.config.dir='/etc/esurfingclient'
	changed=1
}

for key in ; do
	uci -q get "esurfingclient.config.$$key" >/dev/null 2>&1 || continue
	uci delete "esurfingclient.config.$$key"
	changed=1
done

[ "$$changed" = "1" ] && uci commit esurfingclient 2>/dev/null

exit 0
endef

define Package/esurfingclient/postrm
#!/bin/sh
[ "$$PKG_UPGRADE" = "1" ] && exit 0

echo "正在清理 ESurfing 客户端残留文件"

rm -f /etc/config/esurfingclient*

rm -rf /etc/esurfingclient

rm -rf /var/log/esurfing

exit 0
endef

define Package/esurfingclient/description
	Lightweight third-party esurfingclient (Supports multi-platform operation on Windows/Linux/MacOS/OpenWrt)
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/esurfingclient

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/files/etc/init.d/esurfingclient $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/files/etc/config/esurfingclient $(1)/etc/config/esurfingclient

	$(INSTALL_DIR) $(1)/etc/esurfingclient
	$(INSTALL_CONF) $(CURDIR)/files/etc/esurfingclient/ESurfingClient.json $(1)/etc/esurfingclient/ESurfingClient.json
endef

define Build/Compile
	$(call Build/Compile/Default)
endef

$(eval $(call BuildPackage,esurfingclient))
