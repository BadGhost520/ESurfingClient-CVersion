cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROGRAM_VERSION_MAJOR 2)
set(PROGRAM_VERSION_MINOR 0)
set(PROGRAM_VERSION_PATCH 4)
set(PROGRAM_VERSION_RELEASE 1)

set(PROGRAM_VERSION "${PROGRAM_VERSION_MAJOR}.${PROGRAM_VERSION_MINOR}.${PROGRAM_VERSION_PATCH}")
set(PROGRAM_FULL_VERSION "${PROGRAM_VERSION}-r${PROGRAM_VERSION_RELEASE}")

add_definitions(-DPROGRAM_VERSION="${PROGRAM_VERSION}")
add_definitions(-DPROGRAM_FULL_VERSION="${PROGRAM_FULL_VERSION}")

message(STATUS "Building version: ${PROGRAM_FULL_VERSION}")

if (NOT DEFINED __OPENWRT__)
    set(__OPENWRT__ 0)
endif ()

if (__OPENWRT__)
    add_definitions(-D__OPENWRT__=1)
    message(STATUS "OpenWRT - yes")
else ()
    message(STATUS "OpenWRT - no")
endif ()

set(WEB_ROOT_DIR "${CMAKE_SOURCE_DIR}/web_root")
add_definitions(-DWEB_ROOT_PATH="${WEB_ROOT_DIR}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (WIN32)
    add_definitions(-DCURL_STATICLIB)
    add_definitions(-DOPENSSL_NO_DYNAMIC_ENGINE)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

    set(BUILD_SHARED_LIBS OFF)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(CURL_USE_STATIC_LIBS TRUE)

    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif ()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

if (__OPENWRT__)
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
else ()
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
#            src/webserver/WebServer.c
#            src/webserver/mongoose.c
            src/utils/PlatformUtils.c
            src/utils/SimThread.c
            src/utils/Shutdown.c
            src/utils/Service.c
            src/utils/Logger.c
            src/utils/cJSON.c
            src/DialerClient.c
            src/NetClient.c
            src/States.c
            Main.c
    )
endif ()

target_include_directories(ESurfingClient PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

if (WIN32)
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
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
else ()
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
            OpenSSL::Crypto
            pthread
            dl
    )
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(ESurfingClient PRIVATE
                atomic
                m
        )
        if (__OPENWRT__)
            target_link_libraries(ESurfingClient PRIVATE
                    OpenSSL::SSL
            )
        endif ()
    endif ()
endif ()

target_compile_options(ESurfingClient PRIVATE -Os)

target_compile_options(ESurfingClient PRIVATE -Wall -Wextra -Wno-unused-parameter)

if (__OPENWRT__)
    set_target_properties(ESurfingClient PROPERTIES LINK_FLAGS "-Wl,--strip-all")
else ()
    add_custom_command(TARGET ESurfingClient POST_BUILD
        COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:ESurfingClient>"
)
endif ()

install(TARGETS ESurfingClient RUNTIME DESTINATION bin)
