cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

if(WIN32)
    find_package(LibXml2 REQUIRED)
endif()

add_executable(ESurfingClient
        src/DialerApp.c
        src/Client.c
        src/NetworkStatus.c
        src/utils/PlatformUtils.c
        src/Constants.c
        src/headFiles/Constants.h
        src/States.c
        src/headFiles/States.h
        src/NetClient.c
        src/headFiles/Session.h
        src/Session.c
        src/headFiles/cipher/KeyData.h
        src/headFiles/cipher/CipherUtils.h
        src/headFiles/cipher/CipherInterface.h
        src/headFiles/cipher/impl/zuc.h
        src/headFiles/cipher/impl/sm4_ecb.h
        src/headFiles/cipher/impl/sm4_cbc.h
        src/headFiles/cipher/impl/mod_xtea_iv.h
        src/headFiles/cipher/impl/mod_xtea.h
        src/headFiles/cipher/impl/desede_ecb.h
        src/headFiles/cipher/impl/desede_cbc.h
        src/headFiles/cipher/impl/aes_ecb.h
        src/headFiles/cipher/impl/aes_cbc.h
        src/cipher/KeyData.c
        src/cipher/CipherUtils.c
        src/cipher/CipherFactory.c
        src/cipher/impl/zuc.c
        src/cipher/impl/sm4_ecb.c
        src/cipher/impl/sm4_cbc.c
        src/cipher/impl/mod_xtea_iv.c
        src/cipher/impl/mod_xtea.c
        src/cipher/impl/desede_ecb.c
        src/cipher/impl/desede_cbc.c
        src/cipher/impl/aes_ecb.c
        src/cipher/impl/aes_cbc.c
        src/utils/Shutdown.c
        src/headFiles/utils/Shutdown.h
        src/headFiles/Options.h
        src/Options.c
        src/utils/Logger.c
        src/headFiles/utils/Logger.h
        src/cipher/impl/desede_cbc_pc.c
        src/headFiles/cipher/impl/desede_cbc_pc.h
        src/cipher/impl/aes_ecb_pc.c
        src/cipher/impl/aes_cbc_pc.c
        src/cipher/impl/mod_xtea_pc.c
        src/headFiles/cipher/impl/mod_xtea_pc.h
        src/cipher/impl/mod_xtea_cbc_triple_pc.c
        src/headFiles/cipher/impl/mod_xtea_cbc_triple_pc.h
        src/cipher/impl/des_ecb_six_pc.c
        src/headFiles/cipher/impl/aes_cbc_pc.h
        src/headFiles/cipher/impl/aes_ecb_pc.h
        src/headFiles/cipher/impl/des_ecb_six_pc.h
)

if(WIN32)
    target_compile_definitions(ESurfingClient PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(ESurfingClient PRIVATE CURL_STATICLIB)

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    find_library(CURL_STATIC_LIB
            NAMES libcurl.a curl
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(SSL_STATIC_LIB
            NAMES libssl.a ssl
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(CRYPTO_STATIC_LIB
            NAMES libcrypto.a crypto
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(XML2_STATIC_LIB
            NAMES libxml2.a xml2
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(Z_STATIC_LIB
            NAMES libz.a z
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(NGHTTP2_STATIC_LIB
            NAMES libnghttp2.a nghttp2
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(SSH2_STATIC_LIB
            NAMES libssh2.a ssh2
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(ZSTD_STATIC_LIB
            NAMES libzstd.a zstd
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(IDN2_STATIC_LIB
            NAMES libidn2.a idn2
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(PSL_STATIC_LIB
            NAMES libpsl.a psl
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(BROTLI_DEC_STATIC_LIB
            NAMES libbrotlidec.a brotlidec
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(BROTLI_COMMON_STATIC_LIB
            NAMES libbrotlicommon.a brotlicommon
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(UNISTRING_STATIC_LIB
            NAMES libunistring.a unistring
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    find_library(ICONV_STATIC_LIB
            NAMES libiconv.a iconv
            PATHS
            /mingw64/lib
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
            NO_DEFAULT_PATH
    )

    if(NOT CURL_STATIC_LIB OR NOT SSL_STATIC_LIB OR NOT CRYPTO_STATIC_LIB OR NOT XML2_STATIC_LIB OR NOT Z_STATIC_LIB OR NOT NGHTTP2_STATIC_LIB OR NOT SSH2_STATIC_LIB)
        find_package(CURL REQUIRED)
        find_package(OpenSSL REQUIRED)
        find_package(LibXml2 REQUIRED)

        target_link_libraries(ESurfingClient PRIVATE
                CURL::libcurl
                LibXml2::LibXml2
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
    else()
        target_link_libraries(ESurfingClient PRIVATE
                ${CURL_STATIC_LIB}
                ${SSH2_STATIC_LIB}
                ${XML2_STATIC_LIB}
                ${SSL_STATIC_LIB}
                ${CRYPTO_STATIC_LIB}
                ${Z_STATIC_LIB}
                ${NGHTTP2_STATIC_LIB}
        )
        if(ZSTD_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${ZSTD_STATIC_LIB})
        endif()
        if(IDN2_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${IDN2_STATIC_LIB})
        endif()
        if(PSL_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${PSL_STATIC_LIB})
        endif()
        if(BROTLI_DEC_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${BROTLI_DEC_STATIC_LIB})
        endif()
        if(BROTLI_COMMON_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${BROTLI_COMMON_STATIC_LIB})
        endif()
        if(UNISTRING_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${UNISTRING_STATIC_LIB})
        endif()
        if(ICONV_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${ICONV_STATIC_LIB})
        endif()

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
    endif()
    set_target_properties(ESurfingClient PROPERTIES
            LINK_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive"
    )

    set_target_properties(ESurfingClient PROPERTIES
            LINK_SEARCH_START_STATIC ON
            LINK_SEARCH_END_STATIC ON
    )
elseif(UNIX AND NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_STATIC ON CACHE BOOL "Build with static linking")

    find_library(CURL_STATIC_LIB
            NAMES libcurl.a curl
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
    )

    find_library(SSL_STATIC_LIB
            NAMES libssl.a ssl
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
    )

    find_library(CRYPTO_STATIC_LIB
            NAMES libcrypto.a crypto
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
    )

    find_library(Z_STATIC_LIB
            NAMES libz.a z
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
            ${CMAKE_PREFIX_PATH}/lib
    )

    if(BUILD_STATIC AND CURL_STATIC_LIB AND SSL_STATIC_LIB AND CRYPTO_STATIC_LIB)
        target_link_libraries(ESurfingClient PRIVATE
                /usr/local/lib/libcurl.a
                ${SSL_STATIC_LIB}
                ${CRYPTO_STATIC_LIB}
                dl
                pthread
        )
        set_target_properties(ESurfingClient PROPERTIES
                LINK_FLAGS "-static -static-libgcc -static-libstdc++"
        )
    else()
        target_link_libraries(ESurfingClient PRIVATE
                CURL::libcurl
                OpenSSL::SSL
                OpenSSL::Crypto
        )
    endif()
    target_compile_options(ESurfingClient PRIVATE -Wall -Wextra -Wno-unused-parameter)
    set_target_properties(ESurfingClient PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
    )
endif()

include(GNUInstallDirs)
install(TARGETS ESurfingClient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

target_compile_options(ESurfingClient PRIVATE -s)
target_compile_options(ESurfingClient PRIVATE -O3)
set_target_properties(ESurfingClient PROPERTIES LINK_FLAGS "-Wl,--strip-all")