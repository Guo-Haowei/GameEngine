set(TARGET_NAME imnodes)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/imnodes)

file(GLOB SRC
    ${SOURCE_DIR}/imnodes_internal.h
    ${SOURCE_DIR}/imnodes.cpp
    ${SOURCE_DIR}/imnodes.h
)

add_library(${TARGET_NAME} STATIC ${SRC})

target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirdparty/imgui)

source_group("" FILES ${SRC})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER thirdparty)
