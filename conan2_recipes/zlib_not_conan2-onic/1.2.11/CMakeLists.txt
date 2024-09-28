cmake_minimum_required(VERSION 3.19)
project(conanzlib)

include_directories(${CMAKE_SOURCE_DIR}/source_subfolder)
add_subdirectory("source_subfolder")

message(STATUS "BUILDING WITH MINIZIP ${BUILD_MINIZIP}")
if(${BUILD_MINIZIP})
    add_subdirectory("source_subfolder/contrib/minizip")
endif()
