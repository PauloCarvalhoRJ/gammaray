cmake_minimum_required(VERSION 3.19)
project(test_package C)

find_package(bzip2 CONFIG REQUIRED)

ADD_EXECUTABLE(test_bzip2 test_package.c)

# In Conan, this also takes care of header dir finding.  For old standard like using include_dirs(${ZLIB_INCLUDE_DIR}), 
# as well as how one knows that the name of the target is bzip2::bzip2, see https://github.com/conan-io/conan/issues/16504
# NOTE: Check whether this should be BZIP2::BZIP2, especially in Linux.
target_link_libraries(test_bzip2 bzip2::bzip2)
