include $(TOPDIR)/rules.mk

NAME:=remotewol
PKG_NAME:=luci-app-$(NAME)
PKG_VERSION:=0.1
PKG_RELEASE:=2024

LUCI_TITLE:=LuCI Remote Wake On LAN
LUCI_DESCRIPTION:=Wake on lan from router with luci support, wiritten by Acgnu
#LUCI_DEPENDS:=+luci-base
LUCI_PKGARCH:=all

#定义安装目录
define Package/$(PKG_NAME)/install
    $(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(NAME) $(1)/usr/bin/$(NAME)
endef

include $(TOPDIR)/feeds/luci/luci.mk

#下面这行注释不能删, 否则在 make menuconfig 时会看不到这个包
# call BuildPackage - OpenWrt buildroot signature