
set(GL3W_DIR ${CMAKE_SOURCE_DIR}/extern/imgui/examples/libs/gl3w/GL)
set(GL3W_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extern/imgui/examples/libs/gl3w)

set(GL3W_SOURCES
	${GL3W_DIR}/gl3w.c
)

find_package(OpenGL)

add_library(gl3w ${GL3W_SOURCES})
target_include_directories(gl3w PRIVATE ${GL3W_DIR})
target_include_directories(gl3w PUBLIC ${GL3W_INCLUDE_DIR})
target_link_libraries(gl3w PRIVATE ${OPENGL_gl_LIBRARY})

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)

set(SOURCES
	Application.cpp
	Im3D.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/examples/imgui_impl_glfw.cpp
	${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp)
add_library(Gui ${SOURCES})

target_link_libraries(Gui PRIVATE glfw)
target_link_libraries(Gui PRIVATE gl3w)
target_include_directories(Gui PRIVATE ${IMGUI_DIR}/examples)
target_include_directories(Gui PUBLIC include)
target_include_directories(Gui PUBLIC ${IMGUI_DIR})