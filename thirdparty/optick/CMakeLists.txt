file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(optick STATIC ${SRC})
