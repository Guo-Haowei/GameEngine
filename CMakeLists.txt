cmake_minimum_required(VERSION 3.18)
project(GameEngine)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(USE_EMSCRIPTEN "Build for Emscripten/WebAssembly" OFF)
option(BUILD_TINYGLTF_LOADER "Build tinygltf_loader" ON)
option(BUILD_UNIT_TESTS "Build Tests" OFF)

message("***********************************************************************")
message(STATUS "CMAKE_CXX_COMPILER_ID   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_BUILD_TOOL        : ${CMAKE_BUILD_TOOL}")

################################################################################

if (EMSCRIPTEN)
    set(BUILD_TINYGLTF_LOADER OFF CACHE BOOL "" FORCE)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()

if (NOT EMSCRIPTEN)
    # GLFW
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/glfw)
    set_target_properties(glfw PROPERTIES FOLDER thirdparty)
    set_target_properties(update_mappings PROPERTIES FOLDER thirdparty)

    # GLAD
    add_subdirectory(thirdparty/glad)
    set_target_properties(glad PROPERTIES FOLDER thirdparty)
endif()

# GLM
add_subdirectory(thirdparty/glm)
set_target_properties(glm PROPERTIES FOLDER thirdparty)

# YAML-CPP
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES FOLDER thirdparty)

include(cmake/build_bullet.cmake)
include(cmake/build_lua.cmake)
include(cmake/build_imgui.cmake)
include(cmake/build_imnodes.cmake)

if(WIN32)
    message(STATUS "Configuring optick...")
    add_subdirectory(thirdparty/optick)
    set_target_properties(optick PROPERTIES FOLDER thirdparty)
endif()

################################################################################

# gtest
if (BUILD_UNIT_TESTS)
    enable_testing()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/googletest)
    set_target_properties(gtest PROPERTIES FOLDER thirdparty)
    set_target_properties(gtest_main PROPERTIES FOLDER thirdparty)
endif()

function(target_set_warning_level TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /WX)
    elseif(APPLE OR EMSCRIPTEN)
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    else()
        message(FATAL_ERROR "Platform not supported")
    endif()
endfunction()

add_subdirectory(engine)
add_subdirectory(modules)
add_subdirectory(plugins)
add_subdirectory(tools)
add_subdirectory(applications)
