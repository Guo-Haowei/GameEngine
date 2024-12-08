set(LIB_NAME shaders)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hlsl"
)

foreach(FILENAME ${SRC})
    if(${FILENAME} MATCHES "\\.hlsl$")
        set_source_files_properties(${FILENAME} PROPERTIES VS_TOOL_OVERRIDE "None")
    endif()
endforeach()

set(FILTERED_SRC)

foreach(ITEM ${SRC})
    if(NOT ITEM MATCHES "glsl_generated")
        list(APPEND FILTERED_SRC ${ITEM})
    endif()
endforeach()

add_library(${LIB_NAME} STATIC ${FILTERED_SRC})

set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${LIB_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/engine/src/
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${FILTERED_SRC})