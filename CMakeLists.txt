# CMakeList.txt : CMake project for Visualization, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("Visualization")


add_subdirectory(extern/glfw)
add_subdirectory(lib)

# Add source to this project's executable.
set(sources
	Visualization.cpp
	)

add_executable (Visualization  ${sources})
target_link_libraries(Visualization PRIVATE Gui)


# TODO: Add tests and install targets if needed.
