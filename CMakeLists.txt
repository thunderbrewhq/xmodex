cmake_minimum_required(VERSION 3.10...4.4.2)

project(xmodex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# detect which system is being targeted
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(XMODEX_SYSTEM_WIN 1)
    add_definitions(-DXMODEX_SYSTEM_WIN=1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(XMODEX_SYSTEM_MAC 1)
    add_definitions(-DXMODEX_SYSTEM_MAC=1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(XMODEX_SYSTEM_LINUX 1)
    add_definitions(-DXMODEX_SYSTEM_LINUX=1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(XMODEX_SYSTEM_BSD 1)
    add_definitions(-DXMODEX_SYSTEM_BSD=1)
endif()

# detect CPU features
if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    add_definitions(-DXMODEX_CPU_BIGENDIAN=1)
else()
    add_definitions(-DXMODEX_CPU_LITTLEENDIAN=1)
endif()

option(XMODEX_EXAMPLES "Build example programs" OFF)
option(XMODEX_SHARED "Build xmodex as a shared library (not recommended)" OFF)
option(XMODEX_COMPAT "Build experimental FMOD 3.X compatibility into xmodex (not recommended)" OFF)

if(XMODEX_SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_subdirectory(xmodex)
add_subdirectory(vendor)

if(XMODEX_EXAMPLES)
    add_subdirectory(examples)
endif()
