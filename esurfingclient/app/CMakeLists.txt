cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROGRAM_VERSION_MAJOR 2)
set(PROGRAM_VERSION_MINOR 2)
set(PROGRAM_VERSION_PATCH 0)
set(PROGRAM_VERSION_RELEASE 7)

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

add_executable(ESurfingClient
        third_party/cJSON/cJSON.c
        src/cipher/CipherFactory.c
        src/cipher/CipherUtils.c
        src/cipher/KeyData.c
        src/cipher/algo/aes_cbc.c
        src/cipher/algo/aes_common.c
        src/cipher/algo/aes_double_cbc.c
        src/cipher/algo/aes_double_ecb.c
        src/cipher/algo/aes_ecb.c
        src/cipher/algo/des_ecb_six.c
        src/cipher/algo/des_ecb_six_keys.c
        src/cipher/algo/desede_cbc.c
        src/cipher/algo/desede_double_cbc.c
        src/cipher/algo/desede_ecb.c
        src/cipher/algo/mod_xtea.c
        src/cipher/algo/mod_xtea_cbc_triple.c
        src/cipher/algo/mod_xtea_iv.c
        src/cipher/algo/xtea_common.c
        src/cipher/algo/sm4_cbc.c
        src/cipher/algo/sm4_common.c
        src/cipher/algo/sm4_ecb.c
        src/cipher/algo/sm4_variant_cbc.c
        src/cipher/algo/sm4_variant_ecb.c
        src/cipher/algo/snow3g_variant.c
        src/cipher/algo/tea_triple_cbc.c
        src/cipher/algo/tea_triple_ecb.c
        src/cipher/algo/zuc.c
        src/core/Refresh.c
        src/core/States.c
        src/config/ConfigTimeWindow.c
        src/config/ConfigAccounts.c
        src/config/ConfigDefaults.c
        src/config/Config.c
        src/dialer/DialerLocation.c
        src/dialer/DialerSession.c
        src/dialer/ErrorCode.c
        src/dialer/DialerTicket.c
        src/dialer/DialerClient.c
        src/dialer/LogoutState.c
        src/dialer/DialerAuth.c
        src/dialer/DialerNet.c
        src/dialer/DialerServer.c
        src/net/NetClientCurl.c
        src/net/CheckNetwork.c
        src/net/NetClient.c
        src/utils/log/LogControl.c
        src/utils/log/LoggerDir.c
        src/utils/log/LogCore.c
        src/utils/log/LogFile.c
        src/utils/log/LogMeta.c
        src/utils/log/LogDir.c
        src/utils/platform/NetAdapters.c
        src/utils/platform/ProcessInfo.c
        src/utils/platform/ExecPath.c
        src/utils/runtime/Shutdown.c
        src/utils/runtime/Watchdog.c
        src/utils/sim/SimEvpCipher.c
        src/utils/sim/SimEvpDes3.c
        src/utils/sim/SimEvpAes.c
        src/utils/sim/SimEvpMd5.c
        src/utils/sim/SimThread.c
        src/utils/text/BytesUtil.c
        src/utils/text/StrUtil.c
        src/utils/text/Xml.c
        src/utils/time/TimeControl.c
        src/utils/time/TimeUtil.c
        src/Main.c
)

if (NOT __OPENWRT__)
    target_sources(ESurfingClient PRIVATE
            third_party/mongoose/mongoose.c
            src/control/ControlClient.c
            src/control/ControlServer.c
            src/supervisor/SupervisorSignal.c
            src/supervisor/SupervisorChild.c
            src/supervisor/SupervisorState.c
            src/supervisor/SupervisorLoop.c
            src/utils/runtime/Service.c
            src/utils/sim/SimProcess.c
            src/net/NetTraffic.c
            src/utils/runtime/ChildRun.c
            src/webserver/HotspotControl.c
            src/webserver/WebLogFiles.c
            src/webserver/WebApiPost.c
            src/webserver/WebApiGet.c
            src/webserver/WebServer.c
            src/webserver/WebServe.c
    )

    add_custom_target(CopyPortal ALL
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/portal"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/portal"
                "${CMAKE_CURRENT_BINARY_DIR}/portal"
            COMMENT "Copy 'portal' to build dir"
    )

    add_dependencies(ESurfingClient
            CopyPortal
    )
endif ()

target_include_directories(ESurfingClient PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
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
