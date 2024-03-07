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

add_library(${LIB_NAME} STATIC ${SRC})

set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${SRC})