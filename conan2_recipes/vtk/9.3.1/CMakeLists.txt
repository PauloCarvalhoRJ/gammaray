PROJECT(VTK)
cmake_minimum_required(VERSION 3.19)
include(conanbuildinfo.cmake)
conan_basic_setup()

if(MSVC)
    # Remove the flags related to pdb ("Use Program Database")
    string(REGEX REPLACE "/Z[iI7]" ""
        CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    # Add debug information to the *.lib-files files.
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} /Z7")
endif()

add_subdirectory(VTK-8.2.0)
install(FILES FindVTK.cmake DESTINATION .)
