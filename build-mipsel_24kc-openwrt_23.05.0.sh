#!/bin/bash
echo "Downloading SDK"
wget "https://downloads.openwrt.org/releases/23.05.0/targets/ramips/mt7621/openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
echo "Remove openwrt-sdk dir(if had)"
rm -rf openwrt-sdk
echo "Extracting SDK"
tar -xf openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64.tar.xz
echo "Rename SDK dir"
mv openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64 openwrt-sdk
echo "Copy esurfingclient package to dl dir"
tar -czf esurfingclient.tar.gz esurfing
mv esurfingclient.tar.gz openwrt-sdk/dl/
echo "Copy menuconfig and cover .config"
cp menuconfig.config openwrt-sdk/.config
echo "Copy esurfing package to package dir"
cp -r esurfingclient openwrt-sdk/package/
echo "Go to openwrt-sdk dir"
cd openwrt-sdk
echo "Updating feeds"
scripts/feeds update -a
echo "Installing feeds packages"
scripts/feeds install -a
echo "Making curl package"
make package/curl/compile V=sc
echo "Making openssl package"
make package/openssl/compile V=sc
echo "Making esurfingclient package"
make package/esurfingclient/{download,prepare,configure,compile} V=sc -j1
echo "Copy esurfingclient package to run dir"
cp bin/packages/mipsel_24kc/base/esurfingclient_1.0.0-1_mipsel_24kc.ipk ../esurfingclient_1.0.0-1_mipsel_24kc.ipk