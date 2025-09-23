cmake_minimum_required(VERSION 4.0)
project(ESurfingClient C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CURL_DIR "G:/vcpkg/installed/x64-mingw-static/share/curl")

set(CMAKE_TOOLCHAIN_FILE "G:/CLion 2025.2.1/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(VCPKG_TARGET_TRIPLET "x86-mingw-static" CACHE STRING "Vcpkg triplet")

# 使用vcpkg的静态库
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)

add_executable(ESurfingClient
        DialerApp.c
        Client.c
        NetworkStatus.c
        PlatformUtils.c
        Constants.c
        headFiles/Constants.h
        States.c
        headFiles/States.h
        NetClient.c
)

# 链接静态库
target_link_libraries(ESurfingClient PRIVATE CURL::libcurl)
target_link_libraries(ESurfingClient PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(ESurfingClient PRIVATE LibXml2::LibXml2)

# Windows需要的额外静态库
if(WIN32)
    target_link_libraries(ESurfingClient PRIVATE
            ws2_32
            wldap32
            winmm
            crypt32
            normaliz
            secur32
            bcrypt
            iphlpapi
    )
    # 设置MinGW的完全静态链接选项
    target_compile_definitions(ESurfingClient PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties(ESurfingClient PROPERTIES
        LINK_FLAGS "-static -static-libgcc -static-libstdc++"
    )
endif()