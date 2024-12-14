set(TARGET_NAME loader_assimp)

file(GLOB_RECURSE SRC
    "assimp_asset_loader.h"
    "assimp_asset_loader.cpp"
)

add_library(${TARGET_NAME} STATIC ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})

find_package(assimp CONFIG REQUIRED)

target_include_directories(${TARGET_NAME} PRIVATE
    ${VCPKG_INSTALLED_DIR}/x64-windows/include
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/engine/shader
)

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/engine/src/engine/pch.h)

target_link_libraries(${TARGET_NAME} PRIVATE assimp::assimp)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER plugins)
