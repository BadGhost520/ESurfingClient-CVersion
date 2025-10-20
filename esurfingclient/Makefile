include $(TOPDIR)/rules.mk

PKG_NAME:=esurfingclient
PKG_VERSION:=1.0.0
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
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_C_FLAGS="-O3 -s -ffunction-sections -fdata-sections -fvisibility=hidden -DNDEBUG" \
	-DCMAKE_EXE_LINKER_FLAGS="-s -Wl,--gc-sections,--strip-all,--as-needed,--no-export-dynamic" \
	-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

define Package/esurfingclient
	SECTION:=net
	CATEGORY:=Network
	TITLE:=ESurfingClient
	DEPENDS:=+libcurl +libopenssl +ca-bundle
endef

define Package/esurfingclient/description
A lightweight client using libcurl and OpenSSL with enhanced protection.
endef

# 自定义构建后处理
define Build/Compile
	$(call Build/Compile/Default)

	@if [ -f "$(PKG_BUILD_DIR)/ESurfingClient" ]; then \
		\
		$(TARGET_CROSS)strip --strip-all $(PKG_BUILD_DIR)/ESurfingClient || \
		$(TOOLCHAIN_DIR)/bin/$(TARGET_CC:gcc=strip) --strip-all $(PKG_BUILD_DIR)/ESurfingClient || \
		\
		$(TARGET_CROSS)objcopy \
			--remove-section=.comment \
			--remove-section=.note \
			--remove-section=.note.gnu.build-id \
			--remove-section=.gnu.version \
			--remove-section=.gnu.version_r \
			--remove-section=.eh_frame \
			--remove-section=.eh_frame_hdr \
			--remove-section=.gnu.hash \
			$(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || \
		$(TOOLCHAIN_DIR)/bin/$(TARGET_CC:gcc=objcopy) \
			--remove-section=.comment \
			--remove-section=.note \
			--remove-section=.note.gnu.build-id \
			--remove-section=.gnu.version \
			--remove-section=.gnu.version_r \
			--remove-section=.eh_frame \
			--remove-section=.eh_frame_hdr \
			--remove-section=.gnu.hash \
			$(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || \
		\
		$(TARGET_CROSS)objcopy \
			--set-section-flags .text=alloc,load,readonly,code \
			--set-section-flags .data=alloc,load,data \
			--set-section-flags .rodata=alloc,load,readonly,data \
			$(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true; \
		\
		$(TARGET_CROSS)objcopy \
			--add-section .fake1=/dev/null \
			--set-section-flags .fake1=readonly \
			--add-section .fake2=/dev/null \
			--set-section-flags .fake2=alloc \
			$(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true; \
		\
		printf '\x7f\x45\x4c\x46' | dd of=$(PKG_BUILD_DIR)/ESurfingClient bs=1 count=4 conv=notrunc 2>/dev/null || true; \
		\
		$(TARGET_CROSS)strip --strip-unneeded $(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true; \
		\
		touch -t 200101010000 $(PKG_BUILD_DIR)/ESurfingClient; \
		\
		chmod 755 $(PKG_BUILD_DIR)/ESurfingClient; \
		\
		if [ -x "$(PKG_BUILD_DIR)/ESurfingClient" ]; then \
			cp $(PKG_BUILD_DIR)/ESurfingClient $(PKG_BUILD_DIR)/ESurfingClient.orig; \
			python3 -c " \
			import sys; \
			key = 0xAA; \
			with open('$(PKG_BUILD_DIR)/ESurfingClient.orig', 'rb') as f: \
			data = f.read(); \
			with open('$(PKG_BUILD_DIR)/ESurfingClient.enc', 'wb') as f: \
			f.write(bytes([b ^ key for b in data])); \
			" 2>/dev/null; \
		fi; \
		\
		if [ -f "$(PKG_BUILD_DIR)/ESurfingClient.enc" ]; then \
			echo '#!/bin/sh' > $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'TEMP_FILE="/tmp/.$$(basename $$0).$$$$"' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'trap "rm -f $$TEMP_FILE" EXIT' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'tail -n +8 "$$0" | python3 -c "import sys; key=0xAA; data=sys.stdin.buffer.read(); sys.stdout.buffer.write(bytes([b^key for b in data]))" > $$TEMP_FILE 2>/dev/null || exit 1' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'chmod +x $$TEMP_FILE' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'exec $$TEMP_FILE "$$@"' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			echo 'exit 0' >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			cat $(PKG_BUILD_DIR)/ESurfingClient.enc >> $(PKG_BUILD_DIR)/ESurfingClient.wrapper 2>/dev/null || true; \
			chmod +x $(PKG_BUILD_DIR)/ESurfingClient.wrapper; \
			mv $(PKG_BUILD_DIR)/ESurfingClient.wrapper $(PKG_BUILD_DIR)/ESurfingClient; \
		fi; \
		\
		ls -la $(PKG_BUILD_DIR)/ESurfingClient; \
		file $(PKG_BUILD_DIR)/ESurfingClient 2>/dev/null || true; \
	fi
endef

define Package/esurfingclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ESurfingClient $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/esurfingclient.init $(1)/etc/init.d/esurfingclient

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/esurfingclient.config $(1)/etc/config/esurfingclient
endef

$(eval $(call BuildPackage,esurfingclient))
