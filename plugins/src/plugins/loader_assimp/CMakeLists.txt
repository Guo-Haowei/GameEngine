set(TARGET_NAME loader_assimp)

file(GLOB_RECURSE SRC
    "assimp_asset_loader.h"
    "assimp_asset_loader.cpp"
)

add_library(${TARGET_NAME} STATIC ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})

target_include_directories(${TARGET_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/thirdparty/glm
    ${PROJECT_SOURCE_DIR}/thirdparty/assimp/include
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/engine/shader
)

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/engine/src/engine/pch.h)

if (NOT BUILD_ASSIMP)
    target_compile_definitions(${TARGET_NAME} PUBLIC -D_NO_ASSIMP)
else()
    set(LIB_ASSIMP_DEBUG_DIR ${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Debug)
    target_link_libraries(${TARGET_NAME} PRIVATE debug
        ${LIB_ASSIMP_DEBUG_DIR}/assimp-vc143-mtd.lib
        ${LIB_ASSIMP_DEBUG_DIR}/IrrXMLd.lib
        ${LIB_ASSIMP_DEBUG_DIR}/zlibstaticd.lib)

    set(LIB_ASSIMP_RELEASE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Release)
    target_link_libraries(${TARGET_NAME} PRIVATE optimized
        ${LIB_ASSIMP_RELEASE_DIR}/assimp-vc143-mt.lib
        ${LIB_ASSIMP_RELEASE_DIR}/IrrXML.lib
        ${LIB_ASSIMP_RELEASE_DIR}/zlibstatic.lib)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER plugins)
