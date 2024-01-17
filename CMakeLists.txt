cmake_minimum_required(VERSION 3.0.0)
project(alpha VERSION 0.1.0)

set(CMAKE_CXX17_COMPILE_FEATURES "cxx_std_17")
set(CMAKE_CXX_STANDARD 17)

if(NOT EMSCRIPTEN)
    string(APPEND CMAKE_CXX_FLAGS " -w")
    set(OUT_DIR ${CMAKE_SOURCE_DIR}/bin)
else()    
    set(OUT_DIR ${CMAKE_SOURCE_DIR}/bin-web)
endif()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
include_directories(external/glm)
include_directories(external/loguru)
include_directories(external/json)

#include_directories(external/imgui)
#include_directories(external/imgui/examples)


add_subdirectory (engine)
add_subdirectory (game)
add_subdirectory (editor)
add_subdirectory (player)
add_subdirectory (test)

