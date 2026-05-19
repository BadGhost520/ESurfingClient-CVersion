include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-esurfingclient
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
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
	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/controller
	$(INSTALL_DATA) $(CURDIR)/luci/controller/esurfingclient.lua $(1)/tmp/luci-staging/new/controller/

	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/view/esurfingclient
	$(INSTALL_DATA) $(CURDIR)/luci/view/config.js $(1)/tmp/luci-staging/new/view/esurfingclient/

	$(INSTALL_DIR) $(1)/tmp/luci-staging/new/menu.d
	$(INSTALL_DATA) $(CURDIR)/luci/menu.d/esurfingclient.json $(1)/tmp/luci-staging/new/menu.d/

	$(INSTALL_DIR) $(1)/tmp/luci-staging/legacy/controller
	$(INSTALL_DATA) $(CURDIR)/legacy-luci/controller/esurfingclient.lua $(1)/tmp/luci-staging/legacy/controller/

	$(INSTALL_DIR) $(1)/tmp/luci-staging/legacy/view/esurfingclient
	$(INSTALL_DATA) $(CURDIR)/legacy-luci/view/config.htm $(1)/tmp/luci-staging/legacy/view/esurfingclient/
endef

define Package/luci-app-esurfingclient/postinst
#!/bin/sh
echo "正在配置 ESurfing 客户端 LuCI 页面"

if [ -d "/usr/share/luci/menu.d" ]; then
	cp /tmp/luci-staging/new/controller/esurfingclient.lua /usr/lib/lua/luci/controller/
	mkdir -p /www/luci-static/resources/view/esurfingclient
	cp /tmp/luci-staging/new/view/esurfingclient/config.js /www/luci-static/resources/view/esurfingclient/
	cp /tmp/luci-staging/new/menu.d/esurfingclient.json /usr/share/luci/menu.d/
else
	cp /tmp/luci-staging/legacy/controller/esurfingclient.lua /usr/lib/lua/luci/controller/
	mkdir -p /usr/lib/lua/luci/view/esurfingclient
	cp /tmp/luci-staging/legacy/view/esurfingclient/config.htm /usr/lib/lua/luci/view/esurfingclient/
fi

rm -rf /tmp/luci-staging

rm -rf /tmp/luci-*
/etc/init.d/uhttpd restart 2>/dev/null &

echo "LuCI 界面配置完成"
exit 0
endef

$(eval $(call BuildPackage,luci-app-esurfingclient))