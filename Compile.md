# 关于自行编译

### 我还是比较建议使用已有的 github action 来编译的, 如果非要自己编译的话, 就往下看吧

### 自行编译教程很简略, 因为过程完全可以参考 action, 这里就不再细说

## Windows

### 简单一点就是在 Windows 使用 vcpkg 安装 mingw 包, 包括 curl 和 openssl 的

### 然后就使用 vcpkg 的 cmake 配置来让 cmake 能找到这两个包就可以编译了

```shell
# 使用软件: CLion
# 参考 CMake 参数
-DVCPKG_TARGET_TRIPLET=x64-mingw-static \
-DCMAKE_TOOLCHAIN_FILE=G:\Vcpkgs\ESurfingClient\scripts\buildsystems\vcpkg.cmake
```

### 复杂一点的就是跟 action 一样, 用 linux 系统交叉编译

### 1. 确保安装了这些包

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

## Linux

### Linux 的比较简单, 只需要手动编译可以被静态链接的 libopenssl 和 libcurl 即可

### 因为 CMakeLists.txt 只适配调教过的 libopenssl 和 libcurl

### 
