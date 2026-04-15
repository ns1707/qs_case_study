# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.20)

# Define the project name
project(comm_task3)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)

# Add the source files
set(SOURCES
    src/main.cpp
)

# Add Executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/src)

# Linking libraries
 target_link_libraries(${PROJECT_NAME} PRIVATE comm_interface communications utils)