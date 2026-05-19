# 自行编译指南

### 我还是比较建议使用已有的 github action 来编译的

> [!WARNING]
> 必须要使用 `build-all-platforms.yml` 去选择对应架构去编译
> 
> 否则会出现不可预测的错误

### 如果非要自己编译的话, 就往下看吧 (其实就是把 action 各个步骤拆解一下)

### 自行编译教程很简略, 因为过程完全可以按照 action 的步骤去做, 这里就不再细说

## Windows

### `简单一点` 就是在 Windows 使用 vcpkg 安装 mingw 包, 包括 curl 和 openssl 的

### 然后就使用 vcpkg 的 .cmake 配置来让 cmake 能找到这两个包就可以编译了

```shell
# 使用软件: CLion
# 参考 CMake 参数
-DVCPKG_TARGET_TRIPLET=x64-mingw-static \
-DCMAKE_TOOLCHAIN_FILE=G:\Vcpkgs\ESurfingClient\scripts\buildsystems\vcpkg.cmake
```

### `复杂一点` 的就是跟 action 一样, 用 linux 系统交叉编译

### 1. 确保安装了以下软件包

```shell
# 示例系统: Debian 13
sudo apt install -y cmake \
                    make \
                    ninja-build \
                    gcc-mingw-w64-x86-64 \
                    g++-mingw-w64-x86-64 \
                    mingw-w64-tools \
                    upx-ucl \
                    wget \
                    file \
                    zip \
                    perl \
                    libperl-dev
```

### 2. 创建交叉编译工具链

```shell
# 根据自身情况判断路径
cd /path/to/esurfingclient/esurfing

cat > toolchain-mingw64.cmake << 'EOF'
      set(CMAKE_SYSTEM_NAME Windows)
      set(CMAKE_SYSTEM_PROCESSOR x86_64)
      
      set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
      set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
      set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
      
      set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
      
      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
      EOF
```

### 3. 使用指定配置编译安装 libopenssl

```shell
cd /tmp
wget https://github.com/openssl/openssl/releases/download/openssl-4.0.0/openssl-4.0.0.tar.gz
tar -xzf openssl-4.0.0.tar.gz
cd openssl-4.0.0

# 使用指定的配置去编译 libopenssl
./Configure \
    mingw64 \
    --cross-compile-prefix=x86_64-w64-mingw32- \
    --prefix=/usr/x86_64-w64-mingw32 \
    --libdir=lib \
    no-shared \
    no-tests \
    no-docs \
    no-apps \
    no-tls \
    no-bf \
    no-cast \
    no-idea \
    no-rc2 \
    no-rc4 \
    no-rc5 \
    no-seed \
    no-aria \
    no-camellia \
    no-sm2 \
    no-sm3 \
    no-mdc2 \
    no-rmd160 \
    no-whirlpool \
    no-blake2 \
    no-md4 \
    no-dtls \
    no-quic \
    no-sctp \
    no-srp \
    no-zlib \
    no-zstd \
    no-brotli \
    no-ktls \
    no-rdrand \
    no-asan \
    no-msan \
    no-ubsan \
    no-trace \
    no-egd \
    no-krb5kdf \
    no-scrypt \
    no-x942kdf \
    no-x963kdf \
    no-srtp \
    no-ocsp \
    no-cms \
    no-ts

make -j$(nproc)
make install
```

### 4. 使用指定配置编译安装 libcurl 

```shell
cd /tmp
wget https://curl.se/download/curl-8.20.0.tar.gz
tar -xzf curl-8.20.0.tar.gz
cd curl-8.20.0

./configure \
    --host=x86_64-w64-mingw32 \
    --prefix=/usr/x86_64-w64-mingw32 \
    --enable-static \
    --disable-shared \
    --without-ssl \
    --enable-http \
    --enable-proxy \
    --enable-cookies \
    --enable-basic-auth \
    --enable-digest-auth \
    --disable-ftp \
    --disable-file \
    --disable-ldap \
    --disable-ldaps \
    --disable-rtsp \
    --disable-dict \
    --disable-telnet \
    --disable-tftp \
    --disable-pop3 \
    --disable-imap \
    --disable-smb \
    --disable-smtp \
    --disable-gopher \
    --disable-mqtt \
    --disable-websockets \
    --disable-bearer-auth \
    --disable-kerberos-auth \
    --disable-negotiate-auth \
    --disable-ntlm \
    --disable-tls-srp \
    --disable-aws \
    --disable-rtmp \
    --without-brotli \
    --without-zstd \
    --without-libpsl \
    --without-libssh2 \
    --without-librtmp \
    --without-libidn2 \
    --without-nghttp2 \
    --without-ngtcp2 \
    --without-nghttp3 \
    --without-quiche \
    --without-msh3 \
    --without-libgsasl \
    --without-libssh \
    --without-wolfssh \
    --without-hyper \
    --without-zlib \
    --disable-ares \
    --disable-rt \
    --disable-threaded-resolver \
    --disable-ipv6 \
    --disable-manual \
    --disable-docs \
    --disable-verbose \
    --disable-versioned-symbols \
    --disable-sspi

make -j$(nproc)
make install
```

### 5. 编译本程序

```shell
# 根据自身情况判断路径
cd /path/to/esurfingclient/esurfing
          
cmake \
    -G Ninja \
    -B build \
    -S . \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake \
    -DBUILD_SHARED_LIBS=OFF

cmake --build build --target ESurfingClient -j$(nproc)
```

### 6. 然后在 build 目录就能找到 .exe 程序

## Linux

### Linux 的比较简单, 只需要手动编译可以被静态链接的 libopenssl 和 libcurl 即可

### 1. 确保安装了以下软件包

```shell
# 示例系统: Debian 13
sudo apt build-essential \
         cmake \
         wget \
         gcc \
         g++ \
         make \
         upx-ucl \
         binutils \
         file
```

### 2. 使用指定配置编译安装 libopenssl

```shell
cd /tmp
wget https://github.com/openssl/openssl/releases/download/openssl-4.0.0/openssl-4.0.0.tar.gz
tar -xzf openssl-4.0.0.tar.gz
cd openssl-4.0.0

./Configure \
    --prefix=/usr/local \
    no-shared \
    no-tests \
    no-docs \
    no-apps \
    no-tls \
    no-bf \
    no-cast \
    no-idea \
    no-rc2 \
    no-rc4 \
    no-rc5 \
    no-seed \
    no-aria \
    no-camellia \
    no-sm2 \
    no-sm3 \
    no-mdc2 \
    no-rmd160 \
    no-whirlpool \
    no-blake2 \
    no-md4 \
    no-dtls \
    no-quic \
    no-sctp \
    no-srp \
    no-zlib \
    no-zstd \
    no-brotli \
    no-ktls \
    no-rdrand \
    no-asan \
    no-msan \
    no-ubsan \
    no-trace \
    no-egd \
    no-krb5kdf \
    no-scrypt \
    no-x942kdf \
    no-x963kdf \
    no-srtp \
    no-ocsp \
    no-cms \
    no-ts

make -j$(nproc)
make install
```

### 3. 使用指定配置编译安装 libcurl

```shell
wget https://curl.se/download/curl-8.20.0.tar.gz
tar -xzf curl-8.20.0.tar.gz
cd curl-8.20.0

./configure \
    --prefix=/usr/local \
    --enable-static \
    --disable-shared \
    --without-ssl \
    --enable-http \
    --enable-proxy \
    --enable-cookies \
    --enable-basic-auth \
    --enable-digest-auth \
    --disable-ftp \
    --disable-file \
    --disable-ldap \
    --disable-ldaps \
    --disable-rtsp \
    --disable-dict \
    --disable-telnet \
    --disable-tftp \
    --disable-pop3 \
    --disable-imap \
    --disable-smb \
    --disable-smtp \
    --disable-gopher \
    --disable-mqtt \
    --disable-websockets \
    --disable-bearer-auth \
    --disable-kerberos-auth \
    --disable-negotiate-auth \
    --disable-ntlm \
    --disable-tls-srp \
    --disable-aws \
    --disable-rtmp \
    --without-brotli \
    --without-zstd \
    --without-libpsl \
    --without-libssh2 \
    --without-librtmp \
    --without-libidn2 \
    --without-nghttp2 \
    --without-ngtcp2 \
    --without-nghttp3 \
    --without-quiche \
    --without-msh3 \
    --without-libgsasl \
    --without-libssh \
    --without-wolfssh \
    --without-hyper \
    --without-zlib \
    --disable-ares \
    --disable-rt \
    --disable-threaded-resolver \
    --disable-ipv6 \
    --disable-unix-sockets \
    --disable-manual \
    --disable-docs \
    --disable-verbose \
    --disable-versioned-symbols

make -j$(nproc)
make install
```

### 4. 编译本程序

```shell
export CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH

# 根据自身情况判断路径
cd /path/to/esurfingclient/esurfing

cmake .

make -j$(nproc)
```

### 5. 在当前目录就能找到编译出来的程序

## macOS

> [!NOTE]
> 和 Linux 的差不多
> 
> 而且毕竟我不怎么会用 macOS, 所以就不赘述了
> 
> 具体可看相应的工作流

## OpenWRT 主程序包

### OpenWRT 包的编译比较麻烦

### 1. 确保安装了以下软件包

```shell
# 示例系统: Debian 13
sudo apt install -y build-essential \
                    clang \
                    flex \
                    bison \
                    g++ \
                    gawk \
                    gcc-multilib \
                    g++-multilib \
                    gettext \
                    git \
                    libncurses5-dev \
                    libssl-dev \
                    rsync \
                    wget \
                    ca-certificates \
                    unzip \
                    file \
                    zstd \
                    python3-setuptools \
                    swig
```

### 2. 下载需要的 SDK 包

```shell
# 使用的镜像站: https://mirrors.sustech.edu.cn/openwrt/releases/
# 示例 SDK 包面向架构: ramips_mt7621
cd /tmp
wget https://mirrors.sustech.edu.cn/openwrt/releases/24.10.6/targets/ramips/mt7621/openwrt-sdk-24.10.6-ramips-mt7621_gcc-13.3.0_musl.Linux-x86_64.tar.zst
tar -I zstd -xf openwrt-sdk-24.10.6-ramips-mt7621_gcc-13.3.0_musl.Linux-x86_64.tar.zst
```

> [!NOTE]
> 值得一提的是
> 
> 因为 OpenWRT 官方在 ipk 时期不支持 qualcommax_ipq60xx
> 
> 所以作者编译了一个用于编译 qualcommax_ipq60xx ipk 包的 SDK 包
> 
> 下载链接在这: [传送门](https://openlist.xylg.com:20001/d/openwrt-sdk-qualcommax-ipq60xx_gcc-13.3.0_musl.Linux-x86_64.tar.zst?sign=QexHFidwDY6AWxAu9ckCn1flR3R-DzoCKlviA-fOs6w=:0)

### 3. 更新 feeds 源

> [!WARNING]
> 做这一步之前, 如果有版本号要求的话
> 
> 检查 esurfingclient 目录下的 Makefile 文件
> 
> 找到 PKG_VERSION 和 PKG_RELEASE
> 
> 修改成自己要的版本号, 否则包版本号默认是 1.0.0-1 

```shell
cp -r esurfingclient openwrt-sdk/package/

cd openwrt-sdk

scripts/feeds update base
scripts/feeds update packages

scripts/feeds install openssl
scripts/feeds install curl
scripts/feeds install esurfingclient
```

### 4-1. 修改编译配置 (没支持的架构)

> [!WARNING]
> 如果不想折腾作者没支持的架构的话
> 
> 可以前往 4-2 步

```shell
# 参考 .configs 目录的 *.config 文件, 勾选和取消勾选指定选项
# 使用 '/' 可以查找对应选项的位置
make menuconfig
```

### 4-2. 修改编译配置 (已支持的架构)

```shell
make defconfig

# 根据要编译的架构判断使用哪个配置文件
cat ../.configs/ramips_mt7621-ipk.config >> .config

make defconfig
```

### 5. 编译软件包

```shell
# 编译 libopenssl 包
make package/openssl/compile -j$(nproc)

# 编译 libcurl 包
make package/curl/compile -j$(nproc)

# 编译本软件包
make package/esurfingclient/compile -j$(nproc)

# 不想找包的话就用这个来找, 会将包复制到当前目录下
# IPK 包查找
find bin/packages -name "esurfingclient*.ipk" -exec cp {} ./ \;
# APK 包查找
find bin/packages -name "esurfingclient*.apk" -exec cp {} ./ \;
```

### 6. 在 bin/packages 目录就能找到编译出来的包

## OpenWRT LuCI

### 与主程序编译差不多, 但更简单, 具体参考工作流
