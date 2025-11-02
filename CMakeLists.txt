cmake_minimum_required(VERSION 3.23)
project(LearnQL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Collect all header files
file(GLOB_RECURSE LEARNQL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/learnql/*.hpp"
)

# LearnQL is a header-only library
add_library(learnql INTERFACE)

# Register all header files with the target (enables CLion recognition)
target_sources(learnql
    INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${LEARNQL_HEADERS}
)

# Include directories
target_include_directories(learnql
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Main executable
add_executable(LearnQL main.cpp)
target_link_libraries(LearnQL PRIVATE learnql)


