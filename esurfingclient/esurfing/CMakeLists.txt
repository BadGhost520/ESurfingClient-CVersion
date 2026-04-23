cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WEB_ROOT_DIR "${CMAKE_SOURCE_DIR}/web_root")
add_definitions(-DWEB_ROOT_PATH="${WEB_ROOT_DIR}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)
    set(CURL_USE_STATIC_LIBS ON)
    set(OPENSSL_USE_STATIC_LIBS ON)
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

target_link_libraries(ESurfingClient PRIVATE
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
)

if(WIN32)
    target_compile_definitions(ESurfingClient PRIVATE
            _CRT_SECURE_NO_WARNINGS
            CURL_STATICLIB
    )

    target_link_libraries(ESurfingClient PRIVATE
            iphlpapi
            ws2_32
            crypt32
    )

    target_link_options(ESurfingClient PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(UNIX AND NOT WIN32)
    target_link_libraries(ESurfingClient PRIVATE pthread dl)
    target_compile_options(ESurfingClient PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_include_directories(ESurfingClient PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_compile_options(ESurfingClient PRIVATE -O3)
set_target_properties(ESurfingClient PROPERTIES LINK_FLAGS "-Wl,--strip-all")

install(TARGETS ESurfingClient
        RUNTIME DESTINATION bin
)
