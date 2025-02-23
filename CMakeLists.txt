cmake_minimum_required(VERSION 3.10)
project(nvimClipboardSync)

# Find libuv
find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

# Set Debug Flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Set Build Type to Debug by Default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add your source files
add_executable(nvimClipboardSync main.cpp)

# Link libuv
target_include_directories(nvimClipboardSync PRIVATE ${LIBUV_INCLUDE_DIRS})
target_link_libraries(nvimClipboardSync PRIVATE ${LIBUV_LIBRARIES})
target_compile_options(nvimClipboardSync PRIVATE ${LIBUV_CFLAGS_OTHER})
#link spdlog
target_link_libraries(nvimClipboardSync PRIVATE spdlog::spdlog)
