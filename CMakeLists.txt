cmake_minimum_required(VERSION 3.20)

project(nvimClipboardSync)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")


set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/share;${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/lib")

find_package(spdlog REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(libuv REQUIRED)
find_package(argparse REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/share")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/lib/libuv.a")
endif()
