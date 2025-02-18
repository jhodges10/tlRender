include(ExternalProject)

set(PNG_SHARED_LIBS ON)
set(PNG_STATIC_LIBS OFF)
if(NOT BUILD_SHARED_LIBS)
    set(PNG_SHARED_LIBS OFF)
    set(PNG_STATIC_LIBS ON)
endif()

set(PNG_ARGS
    ${TLR_EXTERNAL_ARGS}
    -DCMAKE_INSTALL_LIBDIR=lib
    -DPNG_SHARED=${PNG_SHARED_LIBS}
    -DPNG_STATIC=${PNG_STATIC_LIBS}
    -DPNG_TESTS=OFF
    -DPNG_ARM_NEON=off)
if(CMAKE_CXX_STANDARD)
    set(PNG_ARGS ${PNG_ARGS} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
endif()

ExternalProject_Add(
    PNG
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/PNG
    DEPENDS ZLIB
    URL "http://prdownloads.sourceforge.net/libpng/libpng-1.6.37.tar.gz?download"
    CMAKE_ARGS ${PNG_ARGS})

