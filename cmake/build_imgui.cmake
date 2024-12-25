set(TARGET_NAME imgui)

file(GLOB IMGUI_SRC
    "${PROJECT_SOURCE_DIR}/thirdparty/imgui/*.h"
    "${PROJECT_SOURCE_DIR}/thirdparty/imgui/*.cpp"
)

add_library(${TARGET_NAME} STATIC ${IMGUI_SRC})

source_group("" FILES ${IMGUI_SRC})

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER thirdparty)
