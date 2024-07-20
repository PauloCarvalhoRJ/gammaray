cmake_minimum_required(VERSION 3.19)
project(QtIFWTest CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core)

add_executable(QtIFWTest main.cpp)
target_link_libraries(QtIFWTest Qt5::Core)
