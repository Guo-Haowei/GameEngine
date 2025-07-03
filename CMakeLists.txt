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

option(USE_BULLET "Enable Bullet Physics Engine" ON)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/glfw)

# GLAD
add_subdirectory(thirdparty/glad)

# GLM
add_subdirectory(thirdparty/glm)

# YAML-CPP
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
add_subdirectory(thirdparty/yaml-cpp)

# Bullet
if (USE_BULLET)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(ENABLE_VHACD OFF CACHE BOOL "" FORCE)
    set(USE_GLUT OFF CACHE BOOL "" FORCE)
    set(BUILD_ENET OFF CACHE BOOL "" FORCE)
    set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/bullet3)

    set_target_properties(Bullet2FileLoader PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Collision PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Common PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Dynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Geometry PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3OpenCL_clew PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletCollision PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletDynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletInverseDynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletSoftBody PROPERTIES FOLDER thirdparty)
    set_target_properties(LinearMath PROPERTIES FOLDER thirdparty)
endif()

################################################################################
if(NOT APPLE)
    message(STATUS "Configuring optick...")
    add_subdirectory(thirdparty/optick)
    set_target_properties(optick PROPERTIES FOLDER thirdparty)
endif()

include(cmake/build_lua.cmake)
include(cmake/build_imgui.cmake)
include(cmake/build_imnodes.cmake)

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
