cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C)

set(CMAKE_C_STANDARD 11)
set(CURL_DIR "D:/vcpkg/installed/x64-mingw-static/share/curl/")

# 尝试使用pkg-config查找curl
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CURL QUIET libcurl)
endif()

# 如果pkg-config找不到，则使用vcpkg
if(NOT CURL_FOUND)
    # 设置vcpkg工具链文件
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
    
    # 设置vcpkg triplet
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
    
    # 使用vcpkg的curl
    find_package(CURL REQUIRED)
endif()

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

add_executable(ESurfingClient
        DialerApp.c
        Client.c
        NetworkStatus.c
        PlatformUtils.c
        )

# 链接curl库
if(CURL_FOUND AND PKG_CONFIG_FOUND)
    # 使用pkg-config找到的curl
    target_link_libraries(ESurfingClient ${CURL_LIBRARIES})
    target_include_directories(ESurfingClient PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_options(ESurfingClient PRIVATE ${CURL_CFLAGS_OTHER})
else()
    # 使用vcpkg或标准find_package找到的curl
    target_link_libraries(ESurfingClient CURL::libcurl)
endif()

# 链接OpenSSL库
target_link_libraries(ESurfingClient OpenSSL::SSL OpenSSL::Crypto)

# Windows下需要的额外库
if(WIN32)
    target_link_libraries(ESurfingClient
            ws2_32
            wldap32
            winmm
            crypt32
            normaliz
            secur32
            bcrypt
            iphlpapi
    )
    # 解决MinGW的符号问题
    target_compile_definitions(ESurfingClient PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_options(ESurfingClient PRIVATE -Wl,--allow-multiple-definition -Wl,--unresolved-symbols=ignore-all)
endif()