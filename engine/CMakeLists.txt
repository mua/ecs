set(IMGUI_PATH ${CMAKE_SOURCE_DIR}/external/imgui)

FILE(GLOB IMGUI_SOURCES
${IMGUI_PATH}/*.cpp
${IMGUI_PATH}/examples/imgui_impl_glfw.cpp
${IMGUI_PATH}/examples/imgui_impl_opengl3.cpp
) 

SET ("LIBRARY_SOURCES" ${CMAKE_SOURCE_DIR}/external/loguru/loguru.cpp ${CMAKE_SOURCE_DIR}/external/loguru/loguru.hpp)

FILE(GLOB SOURCES src/*.cpp src/**/*.cpp) 
FILE(GLOB HEADERS include/*.h include/**/*.h) 
FILE(GLOB SHADERS resources/shaders/*.*)

include_directories(include)

include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui/examples)
include_directories(${CMAKE_SOURCE_DIR}/external/tinygltf)

add_library(engine ${HEADERS} ${SOURCES} ${IMGUI_SOURCES} ${LIBRARY_SOURCES} ${SHADERS})

source_group(external\\imgui FILES ${IMGUI_SOURCES})
source_group(external\\libs FILES ${LIBRARY_SOURCES})
source_group(glsl FILES ${SHADERS})


message("--------------------------------")
message(${CMAKE_CURRENT_SOURCE_DIR})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${HEADERS} )
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${SOURCES} )

FILE(GLOB RESOURCE_SOURCES resources/**/*.*) 

message("resources ${RESOURCE_DIR}")

add_custom_target(copy-shader-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/_resources)

add_dependencies(engine copy-shader-files)

if(EMSCRIPTEN) 
    string(APPEND CMAKE_CXX_FLAGS " -s USE_SDL=2 -w")
    string(APPEND CMAKE_CXX_FLAGS " -s USE_GLFW=3")
    string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=1")
    string(APPEND CMAKE_CXX_FLAGS " -s ALLOW_MEMORY_GROWTH=1")
    string(APPEND CMAKE_CXX_FLAGS " -s WASM=1")
#    string(APPEND CMAKE_CXX_FLAGS " -s ERROR_ON_UNDEFINED_SYMBOLS=0") 
    string(APPEND CMAKE_CXX_FLAGS " -s FULL_ES3=1")
    string(APPEND CMAKE_CXX_FLAGS " -s MIN_WEBGL_VERSION=2")        
    string(APPEND CMAKE_CXX_FLAGS " -DLOGURU_STACKTRACES=0")
else()
    include_directories(${CMAKE_SOURCE_DIR}/external/glfw)
    link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
    string(APPEND CMAKE_CXX_FLAGS " -DGLEW_STATIC") 
    #include_directories(${CMAKE_SOURCE_DIR}external/sdl/include)
    #set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/sdl)
    find_package(OpenGL)
    target_link_libraries(engine ${OPENGL_gl_LIBRARY})    
    #include_directories(${CMAKE_SOURCE_DIR}/external/sdl/include)
    #link_directories(${CMAKE_SOURCE_DIR}/external/sdl/lib/x64)
    #target_link_libraries (engine sdl2)

    set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/external/glew)
    set(GLEW_VERBOSE 1)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries (engine ${GLEW_STATIC_LIBRARY_RELEASE} ${CMAKE_SOURCE_DIR}/external/glfw/glfw3.lib)


endif()

if(MSVC)
    target_compile_options(engine PRIVATE "/MP")
endif()