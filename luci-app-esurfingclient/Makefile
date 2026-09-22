include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-esurfingclient
PKG_LICENSE:=Apache-2.0
PKG_LICENSE_FILES:=LICENSE

PKG_VERSION:=2.1.1
PKG_RELEASE:=5

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-esurfingclient
	SECTION:=luci
	CATEGORY:=LuCI
	TITLE:=LuCI interface for ESurfingClient
	DEPENDS:=+esurfingclient
	PKGARCH:=all
endef

define Package/luci-app-esurfingclient/description
ESurfing 客户端的 LuCI 配置页面
endef

define Package/luci-app-esurfingclient/install
	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/view
	$(INSTALL_DATA) $(CURDIR)/rootfs/www/luci-static/resources/view/esurfingclient.js $(1)/tmp/luci-staging/new/view/
	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/menu.d
	$(INSTALL_DATA) $(CURDIR)/rootfs/usr/share/luci/menu.d/luci-app-esurfingclient.json $(1)/tmp/luci-staging/new/menu.d/
	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/acl.d
	$(INSTALL_DATA) $(CURDIR)/rootfs/usr/share/rpcd/acl.d/luci-app-esurfingclient.json $(1)/tmp/luci-staging/new/acl.d/

	$(INSTALL_DIR) $(1)/tmp/luci-staging/legacy/controller
	$(INSTALL_DATA) $(CURDIR)/rootfs-legacy/usr/lib/lua/luci/controller/esurfingclient.lua $(1)/tmp/luci-staging/legacy/controller/
	$(INSTALL_DIR) $(1)/tmp/luci-staging/legacy/view
	$(INSTALL_DATA) $(CURDIR)/rootfs-legacy/usr/lib/lua/luci/view/esurfingclient.htm $(1)/tmp/luci-staging/legacy/view/
endef

define Package/luci-app-esurfingclient/postinst
#!/bin/sh
echo "正在配置 ESurfing 客户端 LuCI"

if [ -f "/www/luci-static/resources/luci.js" ]; then
	mkdir -p /www/luci-static/resources/view
	cp /tmp/luci-staging/new/view/esurfingclient.js /www/luci-static/resources/view/
	mkdir -p /usr/share/luci/menu.d
	cp /tmp/luci-staging/new/menu.d/luci-app-esurfingclient.json /usr/share/luci/menu.d/
	mkdir -p /usr/share/rpcd/acl.d
	cp /tmp/luci-staging/new/acl.d/luci-app-esurfingclient.json /usr/share/rpcd/acl.d/
else
	mkdir -p /usr/lib/lua/luci/controller
	cp /tmp/luci-staging/legacy/controller/esurfingclient.lua /usr/lib/lua/luci/controller/
	mkdir -p /usr/lib/lua/luci/view
	cp /tmp/luci-staging/legacy/view/esurfingclient.htm /usr/lib/lua/luci/view/
fi

rm -rf /tmp/luci-staging

rm -rf /tmp/luci-*
/etc/init.d/uhttpd restart 2>/dev/null &
/etc/init.d/rpcd restart 2>/dev/null &

echo "LuCI 页面配置完成"
exit 0
endef

define Package/luci-app-esurfingclient/postrm
#!/bin/sh
echo "正在移除 ESurfingClient LuCI"

rm -f /usr/lib/lua/luci/controller/esurfingclient.lua
rm -f /www/luci-static/resources/view/esurfingclient.js
rm -f /usr/share/luci/menu.d/luci-app-esurfingclient.json
rm -f /usr/lib/lua/luci/view/esurfingclient.htm

rm -rf /tmp/luci-*
/etc/init.d/uhttpd restart 2>/dev/null &

echo "ESurfingClient LuCI 已移除"
exit 0
endef

define Build/Compile
	true
endef

$(eval $(call BuildPackage,luci-app-esurfingclient))
