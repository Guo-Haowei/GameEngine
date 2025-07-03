set(TARGET_NAME editor)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(APPLE)
    add_executable(${TARGET_NAME} MACOSX_BUNDLE ${SRC})
else()
    add_executable(${TARGET_NAME} ${SRC})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/editor FILES ${SRC})

target_include_directories(${TARGET_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/engine/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${TARGET_NAME} PRIVATE
    engine
    bullet3
    the_aviator_lib
    imnodes
)

set_target_properties(${TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER tools)

target_precompile_headers(${TARGET_NAME} PRIVATE src/editor/pch.h)
target_set_warning_level(${TARGET_NAME})
