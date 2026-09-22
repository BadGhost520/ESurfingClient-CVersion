# x86_64-w64-mingw32 交叉编译工具链
#
# 以前这段内容是内联在 docs/Compile.md 的 heredoc 里的, 用户得照着文档
# 手抄一份到 esurfingclient/main/ 下面。工具链属于构建输入, 不是文档内容,
# 所以搬到这里, 文档只写"怎么用"。
#
# 用法 (在 esurfingclient/main 下):
#   cmake -G Ninja -B build -S . \
#       -DCMAKE_TOOLCHAIN_FILE=../../ci/toolchains/mingw64.cmake \
#       -DBUILD_SHARED_LIBS=OFF

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
