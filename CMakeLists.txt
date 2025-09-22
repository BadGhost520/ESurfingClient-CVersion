cmake_minimum_required(VERSION 3.16)
project(ESurfingClient C)

set(CMAKE_C_STANDARD 11)
set(CURL_DIR "D:/vcpkg/installed/x64-mingw-static/share/curl/")

# ����ʹ��pkg-config����curl
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CURL QUIET libcurl)
endif()

# ���pkg-config�Ҳ�������ʹ��vcpkg
if(NOT CURL_FOUND)
    # ����vcpkg�������ļ�
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
    
    # ����vcpkg triplet
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
    
    # ʹ��vcpkg��curl
    find_package(CURL REQUIRED)
endif()

# ����OpenSSL��
find_package(OpenSSL REQUIRED)

add_executable(ESurfingClient
        DialerApp.c
        Client.c
        NetworkStatus.c
        PlatformUtils.c
        Constants.c
        headFiles/Constants.h
        States.c
        headFiles/States.h
)

# ����curl��
if(CURL_FOUND AND PKG_CONFIG_FOUND)
    # ʹ��pkg-config�ҵ���curl
    target_link_libraries(ESurfingClient ${CURL_LIBRARIES})
    target_include_directories(ESurfingClient PRIVATE ${CURL_INCLUDE_DIRS})
    target_compile_options(ESurfingClient PRIVATE ${CURL_CFLAGS_OTHER})
else()
    # ʹ��vcpkg���׼find_package�ҵ���curl
    target_link_libraries(ESurfingClient CURL::libcurl)
endif()

# ����OpenSSL��
target_link_libraries(ESurfingClient OpenSSL::SSL OpenSSL::Crypto)

# Windows����Ҫ�Ķ����
if(WIN32)
    target_link_libraries(ESurfingClient
            ws2_32
            wldap32
            winmm
            crypt32
            normaliz
            secur32
            bcrypt
            iphlpapi
    )
    # ���MinGW�ķ�������
    target_compile_definitions(ESurfingClient PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_options(ESurfingClient PRIVATE -Wl,--allow-multiple-definition -Wl,--unresolved-symbols=ignore-all)
endif()