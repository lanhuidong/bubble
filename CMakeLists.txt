cmake_minimum_required(VERSION 3.26)

project(bubble VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/Users/lanhuidong/Qt/6.5.3/macos/lib/cmake")

#conan配置
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

include(${CMAKE_SOURCE_DIR}/cmake/conan.cmake)

conan_cmake_configure(REQUIRES libusb/1.0.29
                      REQUIRES spdlog/1.16.0
                      GENERATORS CMakeDeps
                      GENERATORS CMakeToolchain
                      IMPORTS "bin, *.dll -> ./bin"
                      IMPORTS "lib, *.dylib* -> ./bin"
                      OPTIONS *:shared=True)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(libusb REQUIRED)
find_package(spdlog REQUIRED)

qt_standard_project_setup()

add_subdirectory(src/app)