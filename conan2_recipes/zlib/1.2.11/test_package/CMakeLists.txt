cmake_minimum_required(VERSION 3.19)
project(test_package C)

find_package(zlib CONFIG REQUIRED)

add_executable(test_zlib test.c)
# In Conan, this also takes care of header dir finding.  For old standard like using include_dirs(${ZLIB_INCLUDE_DIR}), 
# as well as how one knows that the name of the target is zlib::zlib, see https://github.com/conan-io/conan/issues/16504
# NOTE: Check whether this should be ZLIB::ZLIB, especially in Linux.
target_link_libraries(test_zlib zlib::zlib)
set_target_properties(test_zlib PROPERTIES OUTPUT_NAME "test")

if(WITH_MINIZIP)
  add_executable(test_minizip test_minizip.c)
  target_link_libraries(test_minizip zlib::zlib)
endif()
