set(TARGET_NAME the_aviator)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${TARGET_NAME} ${SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ FILES ${SRC})

target_include_directories(${TARGET_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/engine/src
)

target_link_libraries(${TARGET_NAME} PRIVATE
    engine
    the_aviator_lib
    bullet3
)

set_target_properties(${TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER applications)

target_precompile_headers(${TARGET_NAME} PRIVATE src/pch.h)
target_set_warning_level(${TARGET_NAME})
