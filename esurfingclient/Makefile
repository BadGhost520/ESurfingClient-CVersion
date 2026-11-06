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
	# 触发迁移: 不管新目录有没有文件, 旧配置都覆盖过去
	cp "/etc/config/esurfingclient" "/etc/esurfingclient/ESurfingClient.json"

	# conffiles 保护会把新模板写成 .apk-new / -opkg, 用它覆盖 /etc/config 下的旧 JSON
	if [ -f "/etc/config/esurfingclient.apk-new" ]; then
		cp "/etc/config/esurfingclient.apk-new" "/etc/config/esurfingclient"
		rm -f "/etc/config/esurfingclient.apk-new"
	elif [ -f "/etc/config/esurfingclient-opkg" ]; then
		cp "/etc/config/esurfingclient-opkg" "/etc/config/esurfingclient"
		rm -f "/etc/config/esurfingclient-opkg"
	else
		printf "config main\n\toption dir '/etc/esurfingclient'\n" > /etc/config/esurfingclient
	fi

	echo "旧配置已迁移到 /etc/esurfingclient/ESurfingClient.json"
fi

# UCI 选项维护: 缺的补默认值, 废弃的删掉 (以后加/删选项只改这一段)
if [ -f "/etc/config/esurfingclient" ] && [ "$$(head -c 1 /etc/config/esurfingclient 2>/dev/null)" != "{" ]; then
	changed=0

	# 补齐新选项: 已有值不动, 缺的写默认值
	uci -q get esurfingclient.@main[0].dir >/dev/null 2>&1 || {
		uci set esurfingclient.@main[0].dir='/etc/esurfingclient'
		changed=1
	}

	# 删除已废弃的选项: 旧版本有、新版本不再使用的 key 填在 for 列表里
	for key in ; do
		uci -q get "esurfingclient.@main[0].$$key" >/dev/null 2>&1 || continue
		uci delete "esurfingclient.@main[0].$$key"
		changed=1
	done

	[ "$$changed" = "1" ] && uci commit esurfingclient 2>/dev/null
fi

# 模板与当前文件完全一致时清掉 .apk-new, 避免升级后留下无意义的副本
if [ -f "/etc/config/esurfingclient.apk-new" ]; then
	cmp -s "/etc/config/esurfingclient" "/etc/config/esurfingclient.apk-new" \
		&& rm -f "/etc/config/esurfingclient.apk-new"
fi

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
