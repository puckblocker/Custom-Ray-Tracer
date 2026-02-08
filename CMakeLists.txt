# Check Installed Version
cmake_minimum_required(VERSION 3.22.1)
# Project Name
project(ray_tracer VERSION 1.0.0)

# Find Package
find_package(OpenGL REQUIRED)

# Build
add_executable(ray_tracer 
    src/config.h 
    src/main.cpp 
    src/glad.c)

# Located Header Files
target_include_directories(ray_tracer PRIVATE dependencies)
# Link Libraries
target_link_libraries(ray_tracer glfw3)