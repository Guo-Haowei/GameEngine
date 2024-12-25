cmake_minimum_required(VERSION 3.18)
project(GameEngine)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

message("***********************************************************************")
message(STATUS "CMAKE_CXX_COMPILER_ID   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_BUILD_TOOL        : ${CMAKE_BUILD_TOOL}")

################################################################################

find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(Bullet REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(Stb REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

################################################################################
if(NOT APPLE)
    message(STATUS "Configuring optick...")
    add_subdirectory(thirdparty/optick)
    set_target_properties(optick PROPERTIES FOLDER thirdparty)
endif()

# include(cmake/build-lua.cmake)
include(cmake/build-imgui.cmake)

# gtest
enable_testing()
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/googletest)
set_target_properties(gtest PROPERTIES FOLDER thirdparty)
set_target_properties(gtest_main PROPERTIES FOLDER thirdparty)

function(target_set_warning_level TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE /W4 /WX)
    elseif(APPLE)
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    else()
        message(FATAL_ERROR "Platform not supported")
    endif()
endfunction()

add_subdirectory(engine)
add_subdirectory(plugins)
add_subdirectory(tools)
add_subdirectory(applications)
