cmake_minimum_required(VERSION 3.31)
project(openGL_CMake_Starter)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
)
# Disable glfw tests - otherwise cmake generation fails
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

add_executable(open_gl_starter src/bin/open_gl_starter.cpp)
target_link_libraries(open_gl_starter PRIVATE OpenGL::GL glfw)

add_executable(moving_triangles src/bin/moving_triangles.cpp)
target_link_libraries(moving_triangles PRIVATE OpenGL::GL glfw)
