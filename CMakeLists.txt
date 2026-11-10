cmake_minimum_required (VERSION 3.28.0)
project(PiratesOpenGLRenderer LANGUAGES C CXX)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -v")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld -stdlib=libc++ -lc++abi -Wall -Werror")
endif()

include_directories(glad)
include_directories(glm)
include_directories(imgui-docking)
include_directories(imgui-docking/backends)
# assimp
set(ASSIMP_WARNINGS_AS_ERRORS  OFF CACHE BOOL "ASSIMP WARNINGS AS ERRORS" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Assimp build tests" FORCE)
set(ASSIMP_INSTALL ON CACHE BOOL "Assimp install" FORCE)
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "Assimp install PDB" FORCE)
set(ASSIMP_BUILD_MINIZIP ON CACHE INTERNAL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON)

find_package(assimp REQUIRED)
include_directories(assimp)

add_subdirectory(glfw-3.4)

add_executable(PiratesOpenGLRenderer "Demo.cpp" "glad/glad.c" "Demo.h" "Shader.cpp"
        "Shader.h" "stb_image.h" "Deps/Camera.h" "Deps/Camera.cpp" "VAO.h"
        "VAO.cpp" "VBO.h" "VBO.cpp" "Texture.cpp" "Texture.h"
        "Mesh.h" "Mesh.cpp" "IBO.h" "IBO.cpp"
         "AssimpModel.h" "AssimpModel.cpp" "stb_write_image.h" "PCH.hpp" "ComputeShader.cpp" "ComputeShader.h"
        "stb_image.cpp")
target_compile_features(PiratesOpenGLRenderer PRIVATE cxx_std_20)

target_precompile_headers(PiratesOpenGLRenderer PRIVATE "PCH.hpp")
target_link_libraries(PiratesOpenGLRenderer glfw assimp::assimp) #add assimp later
set_target_properties(
        PiratesOpenGLRenderer PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_custom_target(clean_debug COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean)
