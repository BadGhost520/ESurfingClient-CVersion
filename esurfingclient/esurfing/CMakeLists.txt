cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED __OPENWRT__)
    set(__OPENWRT__ 0)
endif()

if(__OPENWRT__)
    add_definitions(-D__OPENWRT__=1)
    message(STATUS "OpenWrt mode: ENABLED")
else()
    message(STATUS "OpenWrt mode: DISABLED")
endif()

set(WEB_ROOT_DIR "${CMAKE_SOURCE_DIR}/web_root")
add_definitions(-DWEB_ROOT_PATH="${WEB_ROOT_DIR}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
    add_definitions(-DCURL_STATICLIB)
    add_definitions(-DOPENSSL_NO_DYNAMIC_ENGINE)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
elseif(UNIX AND NOT WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -pthread")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(ESurfingClient
        src/cipher/impl/mod_xtea_cbc_triple_pc.c
        src/cipher/impl/des_ecb_six_pc.c
        src/cipher/impl/desede_cbc_pc.c
        src/cipher/impl/mod_xtea_pc.c
        src/cipher/impl/mod_xtea_iv.c
        src/cipher/impl/aes_ecb_pc.c
        src/cipher/impl/aes_cbc_pc.c
        src/cipher/impl/desede_ecb.c
        src/cipher/impl/desede_cbc.c
        src/cipher/impl/mod_xtea.c
        src/cipher/impl/sm4_ecb.c
        src/cipher/impl/sm4_cbc.c
        src/cipher/impl/aes_ecb.c
        src/cipher/impl/aes_cbc.c
        src/cipher/impl/zuc.c
        src/cipher/CipherFactory.c
        src/cipher/CipherUtils.c
        src/cipher/KeyData.c
#        src/webserver/WebServer.c
#        src/webserver/mongoose.c
        src/utils/PlatformUtils.c
        src/utils/SimThread.c
        src/utils/Shutdown.c
        src/utils/Logger.c
        src/utils/cJSON.c
        src/DialerClient.c
        src/NetClient.c
        src/States.c
        Main.c
)

target_include_directories(ESurfingClient PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

if(WIN32)
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
            OpenSSL::SSL
            OpenSSL::Crypto
            ws2_32
            wldap32
            winmm
            crypt32
            normaliz
            secur32
            bcrypt
            iphlpapi
    )
elseif(UNIX AND NOT WIN32)
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
            OpenSSL::SSL
            OpenSSL::Crypto
            pthread
            dl
            atomic
    )
endif()

target_compile_options(ESurfingClient PRIVATE -O3)

if(UNIX AND NOT WIN32)
    target_compile_options(ESurfingClient PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips|mipsel")
    add_compile_options(-mno-mips16)
endif()

set_target_properties(ESurfingClient PROPERTIES LINK_FLAGS "-Wl,--strip-all")

install(TARGETS ESurfingClient RUNTIME DESTINATION bin)
