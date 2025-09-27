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
        src/DialerApp.c
        src/Client.c
        src/NetworkStatus.c
        src/PlatformUtils.c
        src/Constants.c
        src/headFiles/Constants.h
        src/States.c
        src/headFiles/States.h
        src/NetClient.c
        src/headFiles/Session.h
        src/Session.c
        src/utils/ByteArray.c
        src/headFiles/utils/ByteArray.h
        src/headFiles/cipher/key_data.h
        src/headFiles/cipher/cipher_utils.h
        src/headFiles/cipher/cipher_interface.h
        src/headFiles/cipher/impl/zuc.h
        src/headFiles/cipher/impl/sm4_ecb.h
        src/headFiles/cipher/impl/sm4_cbc.h
        src/headFiles/cipher/impl/mod_xtea_iv.h
        src/headFiles/cipher/impl/mod_xtea.h
        src/headFiles/cipher/impl/desede_ecb.h
        src/headFiles/cipher/impl/desede_cbc.h
        src/headFiles/cipher/impl/aes_ecb.h
        src/headFiles/cipher/impl/aes_cbc.h
        src/cipher/key_data.c
        src/cipher/cipher_utils.c
        src/cipher/cipher_factory.c
        src/cipher/impl/zuc.c
        src/cipher/impl/sm4_ecb.c
        src/cipher/impl/sm4_cbc.c
        src/cipher/impl/mod_xtea_iv.c
        src/cipher/impl/mod_xtea.c
        src/cipher/impl/desede_ecb.c
        src/cipher/impl/desede_cbc.c
        src/cipher/impl/aes_ecb.c
        src/cipher/impl/aes_cbc.c
        src/utils/ShutdownHook.c
        src/headFiles/utils/ShutdownHook.h

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