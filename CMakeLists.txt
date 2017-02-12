cmake_minimum_required (VERSION 3.1)
project (libtdamf CXX)

# C++ 11
set (CMAKE_CXX_STANDARD 11)

# Version
set (libtdamf_VERSION_MAJOR 1)
set (libtdamf_VERSION_MINOR 0)

# Debug defines
add_definitions(-DDEBUG)

# Includes
include_directories(src)
add_subdirectory(src)
add_subdirectory(tests)

