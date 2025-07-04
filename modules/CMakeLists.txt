add_subdirectory(opengl_common)
add_subdirectory(opengl)

if (USE_BULLET)
    add_subdirectory(bullet3)
endif()

if (BUILD_TINYGLTF_LOADER)
    add_subdirectory(tinygltf)
endif()