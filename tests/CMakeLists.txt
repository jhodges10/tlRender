add_subdirectory(tlrAppTest)
add_subdirectory(tlrCoreTest)
add_subdirectory(tlrTestLib)
add_subdirectory(tlrtest)
if(TLR_BUILD_GL)
    add_subdirectory(tlrGLTest)
endif()
if(TLR_BUILD_QT)
    add_subdirectory(tlrQtTest)
endif()
