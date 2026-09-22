# 程序自行编译教程

> [!NOTE]
> 教程版本: v2.1.1-r5

### 我还是比较建议使用已有的 github action 来编译的

> [!WARNING]
> 必须要使用 `build-all-platforms.yml` 去选择对应架构去编译
> 
> 否则会出现不可预测的错误

### 如果非要自己编译的话, 就往下看吧 (其实就是把 action 各个步骤拆解一下)

### 自行编译教程很简略, 因为过程完全可以按照 action 的步骤去做, 这里就不再细说

# 仓库目录一览

> [!NOTE]
> 动手之前先认清哪个目录是"包", 哪个是"给包用的输入"。

``` text
esurfingclient/              OpenWrt 包①: 主程序
├── Makefile                 只认包内路径 (CI 会把本目录整个拷进 SDK)
├── LICENSE                  包内自带, 不能引用仓库根的 LICENSE
├── files/                   目标 rootfs 的镜像
│   └── etc/{config,init.d}/ 部署到设备上就是这两个位置
└── main/                    CMake 工程
    ├── CMakeLists.txt        ★ 版本号唯一真相源 (set(PROGRAM_VERSION_*))
    ├── src/                 第一方源码 (cipher/clients/control/states/...)
    ├── include/             第一方头文件, 与 src/ 同构
    ├── portal/              内置网页界面的源
    └── third_party/         vendored: cJSON / mongoose / 7z

luci-app-esurfingclient/     OpenWrt 包②: LuCI 页面 (同样自包含)
├── Makefile  LICENSE
├── rootfs/                  新版 LuCI (JS)
└── rootfs-legacy/           老版 LuCI (Lua/HTM)

ci/                          构建输入, 不是文档内容
├── openwrt/all.config       打包用的 defconfig 片段
└── toolchains/mingw64.cmake Windows 交叉编译工具链
scripts/                     构建脚本 (CI 与本地共用)
├── build-portal.sh          网页资源构建 (下载 + tailwind + CDN 本地化)
└── sync-version.sh          版本号分发 (从 CMakeLists 到各包与页面)
docs/                        文档与截图 (assets/)
```

# Windows

### `简单一点` 就是在 Windows 使用 vcpkg 安装 mingw 包, 包括 curl 的

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

### 2. 直接使用仓库里的工具链文件

```shell
# 工具链文件在 ci/toolchains/mingw64.cmake, 不用再手抄一份到 app/ 下面
cat /path/to/ci/toolchains/mingw64.cmake
```

### 3. 使用指定配置编译安装 libcurl 

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
sudo make install
```

### 4. 编译本程序

```shell
# 根据自身情况判断路径
cd /path/to/esurfingclient/app
          
cmake \
    -G Ninja \
    -B build \
    -S . \
    -DCMAKE_TOOLCHAIN_FILE=../../ci/toolchains/mingw64.cmake \
    -DBUILD_SHARED_LIBS=OFF

cmake --build build --target ESurfingClient -j$(nproc)
```

### 5. 然后在 build 目录就能找到 .exe 程序

# Linux

### Linux 的比较简单, 只需要手动编译可以被静态链接的 libcurl 即可

### 1. 确保安装了以下软件包

```shell
# 示例系统: Debian 13
sudo apt install build-essential \
         cmake \
         wget \
         gcc \
         g++ \
         make \
         upx-ucl \
         binutils \
         file
```

### 2. 使用指定配置编译安装 libcurl

```shell
cd /tmp
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
sudo make install
```

### 3. 编译本程序

```shell
export CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH

# 根据自身情况判断路径
cd /path/to/esurfingclient/app

cmake . -B build

cd build

make -j$(nproc)
```

### 4. 在当前目录就能找到编译出来的程序

# macOS

> [!NOTE]
> 和 Linux 的差不多
> 
> 而且毕竟我不怎么会用 macOS, 所以就不赘述了
> 
> 具体可看相应的工作流

# OpenWRT 主程序包

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
# 示例 SDK 包面向架构: mipsel_24kc
cd /tmp
wget https://mirrors.sustech.edu.cn/openwrt/releases/24.10.8/targets/malta/le/openwrt-sdk-24.10.8-malta-le_gcc-13.3.0_musl.Linux-x86_64.tar.zst
tar -I zstd -xf openwrt-sdk-24.10.8-malta-le_gcc-13.3.0_musl.Linux-x86_64.tar.zst
```

> [!NOTE]
> 如果需要编译 apk 包则需要下载 OpenWRT 25.12.x 及以上版本的 SDK 包

### 3. 更新 feeds 源

> [!WARNING]
> 做这一步之前, 如果有版本号要求的话
> 
> 版本号只有一个源: `esurfingclient/app/CMakeLists.txt` 里的
> `set(PROGRAM_VERSION_MAJOR/MINOR/PATCH/RELEASE ...)` 四行
> 
> 改完在仓库根执行 `scripts/sync-version.sh`, 它会把版本号分发到两个包的 Makefile
> 与 LuCI 页面显示的版本号; 也可以直接 `scripts/sync-version.sh 2.2.0-r1` (等价于先改那四行再分发)
> 
> 两个包的 Makefile 里那两行 PKG_VERSION/PKG_RELEASE 是分发生成的, 不要手改

```shell
cp -r esurfingclient openwrt-sdk/package/

cd openwrt-sdk

scripts/feeds update base
scripts/feeds update packages

scripts/feeds install curl
scripts/feeds install esurfingclient
```

### 4. 修改编译配置

```shell
make defconfig

cat ../ci/openwrt/all.config >> .config

make defconfig
```

### 5. 编译软件包

```shell
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

# OpenWRT LuCI

### 与主程序编译差不多, 但更简单, 具体参考工作流
