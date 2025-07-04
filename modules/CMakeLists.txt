add_subdirectory(opengl_common)
if (WIN32)
    add_subdirectory(opengl4)
elseif (EMSCRIPTEN)
    add_subdirectory(opengles3)
endif()

if (BUILD_BULLET3)
    add_subdirectory(bullet3)
endif()

if (BUILD_TINYGLTF_LOADER)
    add_subdirectory(tinygltf)
endif()