add_subdirectory(src)
add_subdirectory(shader)

if (NOT EMSCRIPTEN)
    add_subdirectory(tests)
endif()
