cmake_minimum_required(VERSION 2.8)
project(xmrig)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

option(WITH_LIBCPUID  "Use Libcpuid" ON)
option(WITH_AEON      "CryptoNight-Lite support" ON)
option(WITH_HTTPD     "HTTP REST API" OFF)
option(WITH_CC_CLIENT "CC Client" ON)
option(WITH_CC_SERVER "CC Server" ON)

include (CheckIncludeFile)
include (cmake/cpu.cmake)

find_package(OpenSSL)
find_package(Boost COMPONENTS system REQUIRED)

if (WIN32)
    set(SOURCES_OS
        res/app.rc
        src/App_win.cpp
        src/Cpu_win.cpp
        src/Mem_win.cpp
        src/Platform_win.cpp)

    add_definitions(/DWIN32)
    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
elseif (APPLE)
    set(SOURCES_OS
        src/App_unix.cpp
        src/Cpu_mac.cpp
        src/Mem_unix.cpp
        src/Platform_mac.cpp)
else()
    set(SOURCES_OS
        src/App_unix.cpp
        src/Cpu_unix.cpp
        src/Mem_unix.cpp
        src/Platform_unix.cpp)

    set(EXTRA_LIBS pthread rt)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    set(EXTRA_LIBS ${EXTRA_LIBS} kvm)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    EXECUTE_PROCESS(COMMAND uname -o COMMAND tr -d '\n' OUTPUT_VARIABLE OPERATING_SYSTEM)
    if (OPERATING_SYSTEM MATCHES "Android")
        set(EXTRA_LIBS ${EXTRA_LIBS} log)
    endif()
endif()

add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DUNICODE)
#add_definitions(/DAPP_DEBUG)
add_definitions(-fexceptions)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(UV REQUIRED)

include(cmake/flags.cmake)

if (WITH_LIBCPUID)
    add_subdirectory(src/3rdparty/libcpuid)

    include_directories(src/3rdparty/libcpuid)
    set(CPUID_LIB cpuid)
    set(SOURCES_CPUID src/Cpu_cpuid.cpp)
else()
    add_definitions(/DXMRIG_NO_LIBCPUID)

    if (XMRIG_ARM)
        set(SOURCES_CPUID src/Cpu_arm.cpp)
    else()
        set(SOURCES_CPUID src/Cpu_stub.cpp)
    endif()
endif()

CHECK_INCLUDE_FILE (syslog.h HAVE_SYSLOG_H)
if (HAVE_SYSLOG_H)
    add_definitions(/DHAVE_SYSLOG_H)
    set(SOURCES_SYSLOG src/log/SysLog.h src/log/SysLog.cpp)
endif()

if (NOT WITH_AEON)
    add_definitions(/DXMRIG_NO_AEON)
endif()

if (WITH_HTTPD)
    find_package(MHD)

    if (MHD_FOUND)
        include_directories(${MHD_INCLUDE_DIRS})
        set(HTTPD_SOURCES src/api/Httpd.h src/api/Httpd.cpp)
    else()
        message(FATAL_ERROR "microhttpd NOT found: use `-DWITH_HTTPD=OFF` to build without http deamon support")
    endif()
else()
    add_definitions(/DXMRIG_NO_HTTPD)
    add_definitions(/DXMRIG_NO_API)
endif()

if (WITH_CC_SERVER)
    find_package(MHD)

    if (MHD_FOUND)
        include_directories(${MHD_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "microhttpd NOT found: use `-DWITH_CC=OFF` to build without CC Server support")
    endif()

    set(SOURCES_CC_SERVER
        src/cc/CCServer.cpp
        src/cc/Service.cpp
        src/cc/Summary.cpp
        src/cc/Httpd.cpp
        src/cc/XMRigCC.cpp
        src/Options.cpp
        src/Platform.cpp)
endif()

if (WITH_CC_CLIENT)
    set(SOURCES_CC_CLIENT
        src/cc/CCClient.cpp)
endif()

if (WITH_CC_SERVER OR WITH_CC_CLIENT)
    set(SOURCES_CC_COMMON
        src/cc/ControlCommand.cpp
        src/cc/ClientStatus.cpp)
else()
    add_definitions(/DXMRIG_NO_CC)
endif()

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

add_library(xmrig_common STATIC
    src/Console.cpp
    src/Mem.cpp
    src/net/Url.cpp
    src/log/ConsoleLog.cpp
    src/log/FileLog.cpp
    src/log/Log.cpp
    src/Cpu.cpp)
add_library(xmrig_crypto STATIC
    src/crypto/c_keccak.c
    src/crypto/c_groestl.c
    src/crypto/c_blake256.c
    src/crypto/c_jh.c
    src/crypto/c_skein.c
    src/crypto/CryptoNight.cpp)
add_library(xmrig_os_dependencies STATIC ${SOURCES_OS} ${SOURCES_SYSLOG})
add_library(xmrig_cpuid STATIC ${SOURCES_CPUID})

if (WITH_CC_SERVER OR WITH_CC_CLIENT)
add_library(xmrig_cc_common STATIC ${SOURCES_CC_COMMON})
endif (WITH_CC_SERVER OR WITH_CC_CLIENT)

add_executable(xmrigMiner
    src/api/Api.cpp
    src/api/ApiState.cpp
    src/api/NetworkState.cpp
    src/App.cpp
    src/net/BoostTlsConnection.cpp
    src/net/BoostTcpConnection.cpp
    src/net/Connection.cpp
    src/net/Client.cpp
    src/net/Job.cpp
    src/net/Network.cpp
    src/net/strategies/DonateStrategy.cpp
    src/net/strategies/FailoverStrategy.cpp
    src/net/strategies/SinglePoolStrategy.cpp
    src/net/SubmitResult.cpp
    src/Summary.cpp
    src/workers/MultiWorker.cpp
    src/workers/Handle.cpp
    src/workers/Hashrate.cpp
    src/workers/Worker.cpp
    src/workers/Workers.cpp
    src/Options.cpp
    src/Platform.cpp
    src/xmrig.cpp
    ${HTTPD_SOURCES}
    ${SOURCES_CC_CLIENT}
    res/app.rc)

target_link_libraries(xmrigMiner
        xmrig_crypto
        xmrig_common
        xmrig_os_dependencies
        xmrig_cpuid
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        ${UV_LIBRARIES}
        ${MHD_LIBRARY}
        ${EXTRA_LIBS}
        ${CPUID_LIB})

if (WITH_CC_CLIENT)
    target_link_libraries(xmrigMiner xmrig_cc_common)
endif (WITH_CC_CLIENT)

add_executable(xmrigDaemon src/cc/XMRigd.cpp res/app.rc)

if (WITH_CC_SERVER AND MHD_FOUND)

    add_executable(xmrigCCServer ${SOURCES_CC_SERVER} res/app.rc)
    target_link_libraries(xmrigCCServer
            xmrig_common
            xmrig_os_dependencies
            xmrig_cpuid
            xmrig_cc_common
            ${UV_LIBRARIES}
            ${MHD_LIBRARY}
            ${EXTRA_LIBS}
            ${CPUID_LIB})
    set_target_properties(xmrigCCServer PROPERTIES COMPILE_FLAGS "-DXMRIG_CC_SERVER ${SHARED_FLAGS}")
endif()

add_subdirectory(test EXCLUDE_FROM_ALL)
