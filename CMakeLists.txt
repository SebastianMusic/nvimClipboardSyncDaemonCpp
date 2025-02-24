cmake_minimum_required(VERSION 3.10)
project(nvimClipboardSync)

# Find libuv
find_package(PkgConfig REQUIRED)

# Set Debug Flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Set Build Type to Debug by Default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

#Find static libraries
find_library(LIBUV_STATIC NAMES libuv.a PATHS /usr/local/lib)
find_library(SPDLOG_STATIC NAMES libspdlog.a PATHS /usr/local/lib)

if(LIBUV_STATIC AND SPDLOG_STATIC)
	message(STATUS "Using static libuv: ${LIBUV_STATIC}")
	message(STATUS "Using static spdlog: ${SPDLOG_STATIC}")
else()
	message(FATAL_ERROR " one or more static libraries not found")
endif()

# Add your source files
add_executable(nvimClipboardSync main.cpp)
target_link_libraries(nvimClipboardSync PRIVATE ${LIBUV_STATIC} ${SPDLOG_STATIC})

# Include headers
include_directories(/usr/local/include)

