#!/bin/bash
filename="openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64.tar.xz"

if [[ -f "$filename" ]]; then
    echo "Had $filename, skip download"
else
    echo "No $filename, Downloading"
    wget "https://mirrors.sustech.edu.cn/openwrt/releases/23.05.0/targets/ramips/mt7621/openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
fi
echo "Remove openwrt-sdk dir(if had)"
rm -rf openwrt-sdk
echo "Extracting SDK"
tar -xf openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64.tar.xz
echo "Rename SDK dir"
mv openwrt-sdk-23.05.0-ramips-mt7621_gcc-12.3.0_musl.Linux-x86_64 openwrt-sdk
echo "Copy esurfingclient package to dl dir"
tar -czf esurfingclient.tar.gz esurfing
mv esurfingclient.tar.gz openwrt-sdk/dl/
echo "Copy esurfing package to package dir"
cp -r esurfingclient openwrt-sdk/package/
echo "Copy menuconfig and cover .config"
cp menuconfig.config openwrt-sdk/.config
echo "Go to openwrt-sdk dir"
cd openwrt-sdk
echo "Updating feeds"
if scripts/feeds update -a; then
  echo "Update feeds successful"
  echo "Installing feeds packages"
  scripts/feeds install -a
else
  echo "Update feeds failed"
  exit 1
fi
echo "Making curl package"
make package/curl/compile V=sc -j$(nproc)
echo "Making openssl package"
make package/openssl/compile V=sc -j$(nproc)
echo "Making esurfingclient package"
make package/esurfingclient/compile V=sc -j$(nproc)
echo "Copy esurfingclient package to bash dir"
ipk_file=$(find openwrt-sdk/bin/packages/mipsel_24kc/base/ -name "esurfingclient_*_mipsel_24kc.ipk" | head -1)
if [ -n "$ipk_file" ]; then
  echo "Found IPK file: $ipk_file"
  cp "$ipk_file" esurfingclient-mipsel_24kc-openwrt-21.02.7.ipk
else
  echo "Error: No esurfingclient IPK file found"
  exit 1
fi