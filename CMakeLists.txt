cmake_minimum_required(VERSION 3.18)
project(GameEngine)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(BUILD_TINYGLTF_LOADER "Build tinygltf_loader" ON)
option(BUILD_UNIT_TESTS "Build Tests" ON)

message("***********************************************************************")
message(STATUS "CMAKE_CXX_COMPILER_ID   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_BUILD_TOOL        : ${CMAKE_BUILD_TOOL}")

################################################################################

if (EMSCRIPTEN)
    set(BUILD_TINYGLTF_LOADER OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(EM_FLAGS "-sUSE_PTHREADS=1 -sSHARED_MEMORY=1")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EM_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EM_FLAGS}")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EM_FLAGS}")

    # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${EM_FLAGS}")
endif ()

if (NOT EMSCRIPTEN)
    # GLAD
    add_subdirectory(thirdparty/glad)
    set_target_properties(glad PROPERTIES FOLDER thirdparty)
endif()

include(cmake/build_glfw.cmake)
include(cmake/build_bullet.cmake)
include(cmake/build_lua.cmake)
include(cmake/build_imgui.cmake)
include(cmake/build_imnodes.cmake)

# GLM
add_subdirectory(thirdparty/glm)
set_target_properties(glm PROPERTIES FOLDER thirdparty)

# YAML-CPP
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES FOLDER thirdparty)

if(WIN32)
    message(STATUS "Configuring optick...")
    add_subdirectory(thirdparty/optick)
    set_target_properties(optick PROPERTIES FOLDER thirdparty)
endif()

################################################################################

# gtest
if (NOT EMSCRIPTEN)
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
