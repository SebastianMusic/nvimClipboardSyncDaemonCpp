cmake_minimum_required(VERSION 3.20)

project(nvimClipboardSync)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

find_package(spdlog REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(libuv REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(argparse REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog uv)
