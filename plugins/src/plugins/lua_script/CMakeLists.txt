set(TARGET_NAME lua_script)

file(GLOB_RECURSE SRC
    "*.h"
    "*.cpp"
    "*.lua"
)

add_library(${TARGET_NAME} STATIC ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})

find_path(LUABRIDGE3_INCLUDE_DIRS "luabridge3/LuaBridge/Array.h")

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LuaJIT REQUIRED IMPORTED_TARGET luajit)

target_include_directories(${TARGET_NAME} PRIVATE
    # ${PROJECT_SOURCE_DIR}/thirdparty/lua/include/lua
    ${PROJECT_SOURCE_DIR}/thirdparty/optick
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/engine/shader
    ${LUABRIDGE3_INCLUDE_DIRS}/luabridge3
)

message(STATUS "******************")
message(STATUS "path is ${LUABRIDGE3_INCLUDE_DIRS}")

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/engine/src/engine/pch.h)

message(STATUS "LuaJIT include directories: ${LUAJIT_INCLUDE_DIRS}")
message(STATUS "LuaJIT libraries: ${LUAJIT_LIBRARIES}")
message(STATUS "LuaJIT version: ${LUAJIT_VERSION}")
target_link_libraries(${TARGET_NAME} PRIVATE PkgConfig::LuaJIT)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER plugins)
