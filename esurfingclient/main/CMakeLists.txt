cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ★ 版本号唯一真相源
#
# 要改版本号就改这四行, 然后跑 scripts/sync-version.sh 分发到各处:
#   - esurfingclient/Makefile           (PKG_VERSION / PKG_RELEASE)
#   - luci-app-esurfingclient/Makefile  (PKG_VERSION / PKG_RELEASE)
#   - 两处 LuCI 页面里显示的版本号
# 那些地方是脚本从这四行生成出来的, 不要手改 (下次同步会被覆盖回去)。
#
# 顺带一个好处: 二进制 -DPROGRAM_FULL_VERSION 报出来的版本与 OpenWrt
# 包版本必然一致 —— 同源。以前 CI 用 sed 现改 Makefile 里的版本号,
# 包版本可以和程序自报的版本对不上。
set(PROGRAM_VERSION_MAJOR 2)
set(PROGRAM_VERSION_MINOR 1)
set(PROGRAM_VERSION_PATCH 1)
set(PROGRAM_VERSION_RELEASE 4)

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

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (WIN32)
    add_definitions(-DCURL_STATICLIB)

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
elseif (ANDROID)
    add_definitions(-DCURL_STATICLIB)

    set(BUILD_SHARED_LIBS OFF)
    set(CURL_USE_STATIC_LIBS TRUE)

    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

    set(BUILD_SHARED_LIBS OFF)
    set(CURL_USE_STATIC_LIBS TRUE)

    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif ()

find_package(CURL REQUIRED)

# 源文件清单只写一遍, 各平台差异在下面用 target_sources 追加。
# (以前 if(__OPENWRT__)/else 各列了 36 行几乎相同的内容, 加一个 .c 要改两处)
add_executable(ESurfingClient
        third_party/7z/LzmaDec.c
        third_party/cJSON/cJSON.c
        src/cipher/algo/linux/mod_xtea_cbc_triple_linux.c
        src/cipher/algo/linux/des_ecb_six_linux.c
        src/cipher/algo/linux/desede_cbc_linux.c
        src/cipher/algo/linux/mod_xtea_linux.c
        src/cipher/algo/linux/aes_ecb_linux.c
        src/cipher/algo/linux/aes_cbc_linux.c
        src/cipher/algo/android/desede_double_cbc_android.c
        src/cipher/algo/android/sm4_variant_cbc_android.c
        src/cipher/algo/android/sm4_variant_ecb_android.c
        src/cipher/algo/android/snow3g_variant_android.c
        src/cipher/algo/android/tea_triple_cbc_android.c
        src/cipher/algo/android/tea_triple_ecb_android.c
        src/cipher/algo/android/aes_double_cbc_android.c
        src/cipher/algo/android/aes_double_ecb_android.c
        src/cipher/algo/android/des_ecb_six_android.c
        src/cipher/algo/ios/ios_zsm.c
        src/cipher/CipherFactory.c
        src/cipher/CipherUtils.c
        src/cipher/KeyData.c
        src/states/Refresh.c
        src/states/States.c
        src/utils/sim/SimThread.c
        src/utils/sim/SimEvp.c
        src/utils/PlatformUtils.c
        src/utils/TimeControl.c
        src/utils/Watchdog.c
        src/utils/LogoutState.c
        src/utils/Shutdown.c
        src/utils/Logger.c
        src/clients/dialer/DialerClient.c
        src/clients/net/CheckNetwork.c
        src/clients/net/NetClient.c
        src/main.c
)

# OpenWrt 那一支不带 Web 界面 / 守护进程编排, 差异就是这 6 个文件。
if (NOT __OPENWRT__)
    target_sources(ESurfingClient PRIVATE
            third_party/mongoose/mongoose.c
            src/supervisor/Supervisor.c
            src/webserver/WebServer.c
            src/control/Control.c
            src/utils/sim/SimProcess.c
            src/utils/Service.c
    )

    add_custom_target(CopyPortal ALL
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/portal"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/portal"
                "${CMAKE_CURRENT_BINARY_DIR}/portal"
            COMMENT "Copy 'portal' to build dir"
    )

    add_dependencies(ESurfingClient CopyPortal)
endif ()

# include/ 是第一方头文件的根, third_party/ 是 vendored 库的根。
# 第一方代码统一写 #include "utils/Logger.h" 这种模块路径,
# vendored 库统一用尖括号 #include <cJSON/cJSON.h>, 一眼能分清是谁的。
target_include_directories(ESurfingClient PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
)

if (WIN32)
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
            ws2_32
            wldap32
            winmm
            crypt32
            normaliz
            secur32
            bcrypt
            iphlpapi
    )
elseif (ANDROID)
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
    )
else ()
    target_link_libraries(ESurfingClient PRIVATE
            CURL::libcurl
            pthread
            dl
    )
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(ESurfingClient PRIVATE
                atomic
                m
        )
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