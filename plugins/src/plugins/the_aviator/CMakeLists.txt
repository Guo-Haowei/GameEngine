set(TARGET_NAME the_aviator_lib)

file(GLOB_RECURSE SRC
    "*.h"
    "*.cpp"
)

add_library(${TARGET_NAME} STATIC ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})

# @TODO: fix this
target_include_directories(${TARGET_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/thirdparty/glm
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/engine/shader
)

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/engine/src/engine/pch.h)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER plugins)
