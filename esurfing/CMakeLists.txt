cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 通用依赖（跨平台）
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Windows 额外依赖（Linux 不强制要求）
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

# Windows平台特殊配置
if(WIN32)
    # 设置编译定义
    target_compile_definitions(ESurfingClient PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(ESurfingClient PRIVATE CURL_STATICLIB)

    # 强制使用静态库路径
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

    # 尝试多个可能的路径查找静态库
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

    # 如果找不到静态库，回退到使用CMake的find_package
    if(NOT CURL_STATIC_LIB OR NOT SSL_STATIC_LIB OR NOT CRYPTO_STATIC_LIB OR NOT XML2_STATIC_LIB OR NOT Z_STATIC_LIB OR NOT NGHTTP2_STATIC_LIB OR NOT SSH2_STATIC_LIB)
        message(STATUS "Some static libraries not found, using find_package fallback")
        if(NOT CURL_STATIC_LIB)
            message(STATUS "Missing: libcurl.a")
        endif()
        if(NOT SSL_STATIC_LIB)
            message(STATUS "Missing: libssl.a")
        endif()
        if(NOT CRYPTO_STATIC_LIB)
            message(STATUS "Missing: libcrypto.a")
        endif()
        if(NOT XML2_STATIC_LIB)
            message(STATUS "Missing: libxml2.a")
        endif()
        if(NOT Z_STATIC_LIB)
            message(STATUS "Missing: libz.a")
        endif()
        if(NOT NGHTTP2_STATIC_LIB)
            message(STATUS "Missing: libnghttp2.a")
        endif()
        if(NOT SSH2_STATIC_LIB)
            message(STATUS "Missing: libssh2.a")
        endif()

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
        # 使用找到的静态库
        message(STATUS "✅ Found all core static libraries, using static linking")
        message(STATUS "CURL: ${CURL_STATIC_LIB}")
        message(STATUS "SSL: ${SSL_STATIC_LIB}")
        message(STATUS "CRYPTO: ${CRYPTO_STATIC_LIB}")
        message(STATUS "XML2: ${XML2_STATIC_LIB}")
        message(STATUS "Z: ${Z_STATIC_LIB}")
        message(STATUS "NGHTTP2: ${NGHTTP2_STATIC_LIB}")
        message(STATUS "SSH2: ${SSH2_STATIC_LIB}")

        # Optional libraries
        if(ZSTD_STATIC_LIB)
            message(STATUS "ZSTD: ${ZSTD_STATIC_LIB}")
        endif()
        if(IDN2_STATIC_LIB)
            message(STATUS "IDN2: ${IDN2_STATIC_LIB}")
        endif()
        if(PSL_STATIC_LIB)
            message(STATUS "PSL: ${PSL_STATIC_LIB}")
        endif()
        if(BROTLI_DEC_STATIC_LIB)
            message(STATUS "BROTLI_DEC: ${BROTLI_DEC_STATIC_LIB}")
        endif()
        if(BROTLI_COMMON_STATIC_LIB)
            message(STATUS "BROTLI_COMMON: ${BROTLI_COMMON_STATIC_LIB}")
        endif()
        if(UNISTRING_STATIC_LIB)
            message(STATUS "UNISTRING: ${UNISTRING_STATIC_LIB}")
        endif()
        if(ICONV_STATIC_LIB)
            message(STATUS "ICONV: ${ICONV_STATIC_LIB}")
        endif()

        # Link core libraries
        target_link_libraries(ESurfingClient PRIVATE
                ${CURL_STATIC_LIB}
                ${SSH2_STATIC_LIB}
                ${XML2_STATIC_LIB}
                ${SSL_STATIC_LIB}
                ${CRYPTO_STATIC_LIB}
                ${Z_STATIC_LIB}
                ${NGHTTP2_STATIC_LIB}
        )

        # Link optional compression and IDN libraries
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

        # Link Unicode and character encoding libraries (required by IDN2 and PSL)
        if(UNISTRING_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${UNISTRING_STATIC_LIB})
        endif()
        if(ICONV_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${ICONV_STATIC_LIB})
        endif()

        # Add Windows system libraries
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

    # 设置MinGW的完全静态链接选项
    set_target_properties(ESurfingClient PROPERTIES
            LINK_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive"
    )
    # 强制使用静态库
    set_target_properties(ESurfingClient PROPERTIES
            LINK_SEARCH_START_STATIC ON
            LINK_SEARCH_END_STATIC ON
    )
    # Linux 静态编译配置
elseif(UNIX AND NOT WIN32)
    # 设置查找静态库
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_STATIC ON CACHE BOOL "Build with static linking")

    # 查找 GSSAPI/Kerberos 相关库
    find_library(GSSAPI_LIB
            NAMES gssapi_krb5 gssapi
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
    )

    find_library(KRB5_LIB
            NAMES krb5
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
    )

    find_library(K5CRYPTO_LIB
            NAMES k5crypto
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
    )

    find_library(COM_ERR_LIB
            NAMES com_err
            PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
            /usr/lib
            /usr/local/lib
    )

    # 寻找静态库
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
        message(STATUS "✅ Building with static libraries on Linux")
        message(STATUS "CURL: ${CURL_STATIC_LIB}")
        message(STATUS "SSL: ${SSL_STATIC_LIB}")
        message(STATUS "CRYPTO: ${CRYPTO_STATIC_LIB}")

        # 链接静态库
        target_link_libraries(ESurfingClient PRIVATE
                ${CURL_STATIC_LIB}
                ${SSL_STATIC_LIB}
                ${CRYPTO_STATIC_LIB}
        )

        # 强制添加 GSSAPI/Kerberos 相关库
        target_link_libraries(ESurfingClient PRIVATE
                gssapi_krb5
                krb5
                k5crypto
                com_err
        )

        # 添加其他可能的依赖
        if(Z_STATIC_LIB)
            target_link_libraries(ESurfingClient PRIVATE ${Z_STATIC_LIB})
        endif()

        # 静态链接标志
        set_target_properties(ESurfingClient PROPERTIES
                LINK_FLAGS "-static -static-libgcc -static-libstdc++"
        )
        # 打印链接状态
        message(STATUS "Linked GSSAPI/Kerberos libraries")
    else()
        message(STATUS "ℹ️  Using dynamic linking on Linux")
        # 动态链接回退
        target_link_libraries(ESurfingClient PRIVATE
                CURL::libcurl
                OpenSSL::SSL
                OpenSSL::Crypto
        )

        # 如果静态库查找失败，给出提示
        if(BUILD_STATIC)
            message(WARNING "Static libraries not found, falling back to dynamic linking")
            if(NOT CURL_STATIC_LIB)
                message(STATUS "Missing: libcurl.a")
            endif()
            if(NOT SSL_STATIC_LIB)
                message(STATUS "Missing: libssl.a")
            endif()
            if(NOT CRYPTO_STATIC_LIB)
                message(STATUS "Missing: libcrypto.a")
            endif()
        endif()
    endif()
    # Linux 编译选项与输出目录
    target_compile_options(ESurfingClient PRIVATE -Wall -Wextra -Wno-unused-parameter)
    set_target_properties(ESurfingClient PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
    )
endif()

include(GNUInstallDirs)
install(TARGETS ESurfingClient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# 去除调试信息
target_compile_options(ESurfingClient PRIVATE -s)
# 优化代码结构
target_compile_options(ESurfingClient PRIVATE -O3)
# 去除符号表
set_target_properties(ESurfingClient PROPERTIES LINK_FLAGS "-Wl,--strip-all")