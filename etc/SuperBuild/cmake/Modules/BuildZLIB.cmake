include(ExternalProject)

set(ZLIB_ARGS ${TLR_EXTERNAL_ARGS})
if(CMAKE_CXX_STANDARD)
    set(ZLIB_ARGS ${ZLIB_ARGS} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
endif()

ExternalProject_Add(
    ZLIB
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZLIB
    URL http://www.zlib.net/zlib-1.2.11.tar.gz
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/ZLIB-patch/CMakeLists.txt
        ${CMAKE_CURRENT_BINARY_DIR}/ZLIB/src/ZLIB/CMakeLists.txt
    CMAKE_ARGS ${ZLIB_ARGS})
