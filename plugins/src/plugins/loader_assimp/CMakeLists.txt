set(TARGET_NAME loader_assimp)

file(GLOB_RECURSE SRC
    "assimp_asset_loader.h"
    "assimp_asset_loader.cpp"
)

add_library(${TARGET_NAME} STATIC ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})

# find_package(assimp CONFIG REQUIRED)

target_include_directories(${TARGET_NAME} PRIVATE
    ${VCPKG_INSTALLED_DIR}/x64-windows/include
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/engine/shader
    ${PROJECT_SOURCE_DIR}/thirdparty/assimp/include
)

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/engine/src/engine/pch.h)

set(lib_assimp_release "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Release/assimp-vc143-mt.lib")
set(lib_irrxmld_release "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Release/IrrXML.lib")
set(lib_zlib_release "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Release/zlibstatic.lib")
target_link_libraries(${TARGET_NAME} optimized ${lib_assimp_release} ${lib_irrxmld_release} ${lib_zlib_release})

set(lib_assimp_debug "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Debug/assimp-vc143-mtd.lib")
set(lib_irrxmld_debug "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Debug/IrrXMLd.lib")
set(lib_zlib_debug "${PROJECT_SOURCE_DIR}/thirdparty/assimp/build/lib/Debug/zlibstaticd.lib")
target_link_libraries(${TARGET_NAME} debug ${lib_assimp_debug} ${lib_irrxmld_debug} ${lib_zlib_debug})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER plugins)
