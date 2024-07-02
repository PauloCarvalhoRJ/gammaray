cmake_minimum_required(VERSION 3.19)
project(PackageTest CXX)

find_package(eigen3 CONFIG REQUIRED)

add_executable(example example.cpp)

# In Conan, this also takes care of header dir finding.  For old standard like using include_dirs(${ZLIB_INCLUDE_DIR}), 
# as well as how one knows that the name of the target is bzip2::bzip2, see https://github.com/conan-io/conan/issues/16504
# NOTE: Check whether this should be BZIP2::BZIP2, especially in Linux.
target_link_libraries(example eigen3::eigen3)
