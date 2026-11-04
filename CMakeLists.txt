cmake_minimum_required(VERSION 3.23)

project(Argus VERSION 0.1.0 LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "Argus requires Windows and the MSVC toolchain.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Argus currently supports x64 builds only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ArgusCompilerOptions INTERFACE)
target_compile_features(ArgusCompilerOptions INTERFACE cxx_std_17)
target_compile_options(ArgusCompilerOptions INTERFACE /W4 /permissive- /Zc:__cplusplus /utf-8)

include(cmake/ArgusWDK.cmake)

add_subdirectory(Core)
add_subdirectory(Agent)
add_subdirectory(Kernel)
