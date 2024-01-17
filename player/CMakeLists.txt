cmake_minimum_required(VERSION 3.5)
FILE(GLOB SOURCES
    src/*.cpp
    src/*.h
    src/**/*.cpp
    src/**/*.h
) 

include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui/examples)
include_directories(${CMAKE_SOURCE_DIR}/engine/include)
include_directories(${CMAKE_SOURCE_DIR}/game/src)

include(GenerateExportHeader)

add_executable(player ${SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX source FILES ${SOURCES})

target_link_libraries (player engine)
target_include_directories(player PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if(EMSCRIPTEN) 
    #string(APPEND CMAKE_CXX_FLAGS " -s USE_SDL=2") 
    string(APPEND CMAKE_CXX_FLAGS " -s USE_GLFW=3")
    string(APPEND CMAKE_CXX_FLAGS " -s FULL_ES3=1")
    string(APPEND CMAKE_CXX_FLAGS " -s MIN_WEBGL_VERSION=2")        
    string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=1")
    string(APPEND CMAKE_CXX_FLAGS " -s ALLOW_MEMORY_GROWTH=1")
    string(APPEND CMAKE_CXX_FLAGS " -s WASM=1")
#    string(APPEND CMAKE_CXX_FLAGS " -s ERROR_ON_UNDEFINED_SYMBOLS=0")    
    string(APPEND CMAKE_CXX_FLAGS " -DIMGUI_DISABLE_FILE_FUNCTIONS")        
    string(APPEND CMAKE_EXE_LINKER_FLAGS " --no-heap-copy --preload-file ${CMAKE_SOURCE_DIR}/external/imgui/misc/fonts@/fonts --preload-file ${CMAKE_SOURCE_DIR}/bin/resources@/resources --preload-file ${CMAKE_SOURCE_DIR}/bin/_resources@/_resources")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    #include_directories(${CMAKE_SOURCE_DIR}/external/emscripten/system/include/SDL)
else()
    include_directories(${CMAKE_SOURCE_DIR}/external/glfw)
    link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
    #include_directories(${CMAKE_SOURCE_DIR}external/sdl/include)
    #set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/sdl)
    find_package(OpenGL)
    target_link_libraries(player ${OPENGL_gl_LIBRARY})    
    #include_directories(${CMAKE_SOURCE_DIR}/external/sdl/include)
    #target_link_directories(editor PUBLIC ${CMAKE_SOURCE_DIR}/external/sdl/lib/x64)
    #target_link_libraries (editor glfw3.lib)
    #target_link_libraries (editor SDL2main)

    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries (player ${GLEW_STATIC_LIBRARY_RELEASE})
    if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        set_property(TARGET player PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
    else()
        set_property(TARGET player PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
    endif()
endif() 

if(MSVC)
    target_compile_options(player PRIVATE "/MP")
endif()

