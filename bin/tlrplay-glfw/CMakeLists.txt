set(HEADERS
    App.h
    Util.h)
set(SOURCE
    App.cpp
    Util.cpp
    main.cpp)

add_executable(tlrplay-glfw ${SOURCE} ${HEADERS})
target_link_libraries(tlrplay-glfw tlrGL tlrApp GLFW)

install(
    TARGETS tlrplay-glfw
    RUNTIME DESTINATION bin)
set_target_properties(tlrplay-glfw PROPERTIES FOLDER bin)
