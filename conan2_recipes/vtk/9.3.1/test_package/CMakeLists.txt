cmake_minimum_required(VERSION 3.19)
project(VTKTest CXX)

find_package(VTK REQUIRED CONFIG)

add_executable(${PROJECT_NAME} test.cpp)
target_link_libraries(${PROJECT_NAME} vtk::vtk)
