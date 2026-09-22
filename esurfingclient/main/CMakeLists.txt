cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROGRAM_VERSION_MAJOR 2)
set(PROGRAM_VERSION_MINOR 1)
set(PROGRAM_VERSION_PATCH 1)
set(PROGRAM_VERSION_RELEASE 5)

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
        third_party/7z/LzmaDec.c
        third_party/cJSON/cJSON.c
        src/cipher/CipherFactory.c
        src/cipher/CipherUtils.c
        src/cipher/KeyData.c
        src/cipher/algo/android/aes_double_cbc_android.c
        src/cipher/algo/android/aes_double_ecb_android.c
        src/cipher/algo/android/des_ecb_six_android.c
        src/cipher/algo/android/desede_double_cbc_android.c
        src/cipher/algo/android/sm4_variant_cbc_android.c
        src/cipher/algo/android/sm4_variant_ecb_android.c
        src/cipher/algo/android/snow3g_variant_android.c
        src/cipher/algo/android/tea_triple_cbc_android.c
        src/cipher/algo/android/tea_triple_ecb_android.c
        src/cipher/algo/ios/ios_zsm.c
        src/cipher/algo/linux/aes_cbc_linux.c
        src/cipher/algo/linux/aes_ecb_linux.c
        src/cipher/algo/linux/des_ecb_six_linux.c
        src/cipher/algo/linux/desede_cbc_linux.c
        src/cipher/algo/linux/mod_xtea_cbc_triple_linux.c
        src/cipher/algo/linux/mod_xtea_linux.c
        src/clients/dialer/dialer_auth.c
        src/clients/dialer/dialer_client.c
        src/clients/dialer/dialer_location.c
        src/clients/dialer/dialer_net.c
        src/clients/dialer/dialer_session.c
        src/clients/dialer/dialer_ticket.c
        src/clients/net/CheckNetwork.c
        src/clients/net/netclient.c
        src/clients/net/netclient_curl.c
        src/config/config.c
        src/config/config_accounts.c
        src/config/config_defaults.c
        src/config/config_time_window.c
        src/main.c
        src/states/Refresh.c
        src/states/States.c
        src/utils/LogoutState.c
        src/utils/Shutdown.c
        src/utils/TimeControl.c
        src/utils/Watchdog.c
        src/utils/bytes_util.c
        src/utils/exec_path.c
        src/utils/log_control.c
        src/utils/log_core.c
        src/utils/log_dir.c
        src/utils/log_file.c
        src/utils/log_meta.c
        src/utils/logger_dir.c
        src/utils/net_adapters.c
        src/utils/process_info.c
        src/utils/sim/SimThread.c
        src/utils/sim/sim_evp_aes.c
        src/utils/sim/sim_evp_cipher.c
        src/utils/sim/sim_evp_des3.c
        src/utils/sim/sim_evp_md5.c
        src/utils/str_util.c
        src/utils/time_util.c
        src/utils/xml.c
)

if (NOT __OPENWRT__)
    target_sources(ESurfingClient PRIVATE
            third_party/mongoose/mongoose.c
            src/control/control_client.c
            src/control/control_server.c
            src/supervisor/supervisor_child.c
            src/supervisor/supervisor_loop.c
            src/supervisor/supervisor_signal.c
            src/supervisor/supervisor_state.c
            src/utils/Service.c
            src/utils/sim/SimProcess.c
            src/webserver/web_api_get.c
            src/webserver/web_api_post.c
            src/webserver/web_log_files.c
            src/webserver/web_serve.c
            src/webserver/web_server.c
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