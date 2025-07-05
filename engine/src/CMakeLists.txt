set(TARGET_NAME engine)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.natvis"
)
set(FILTERED_SRC "")

foreach(file ${SRC})
    if(WIN32)
        if(file MATCHES "apple" OR file MATCHES "metal" )
            # filtered
        else()
            list(APPEND FILTERED_SRC ${file})
        endif()
    elseif(APPLE)
        if(file MATCHES "d3d" OR file MATCHES "windows" OR file MATCHES "vk")
            # filtered
        else()
            list(APPEND FILTERED_SRC ${file})
        endif()
    elseif(EMSCRIPTEN)
        if(file MATCHES "d3d" OR file MATCHES "windows" OR file MATCHES "vk" OR file MATCHES "metal")
            # filtered
        else()
            list(APPEND FILTERED_SRC ${file})
        endif()
    else()
        message(FATAL_ERROR "Platform not supported")
    endif()
endforeach()

add_library(${TARGET_NAME} STATIC ${FILTERED_SRC})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/engine FILES ${FILTERED_SRC})

target_include_directories(${TARGET_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/thirdparty/imgui
    ${PROJECT_SOURCE_DIR}/thirdparty/optick
    ${PROJECT_SOURCE_DIR}/thirdparty/glm
    ${PROJECT_SOURCE_DIR}/thirdparty/
    ${PROJECT_SOURCE_DIR}/engine/shader
    ${PROJECT_SOURCE_DIR}/engine/src
    ${PROJECT_SOURCE_DIR}/plugins/src
    ${PROJECT_SOURCE_DIR}/
)

target_compile_definitions(${TARGET_NAME} PUBLIC
    -D_CRT_SECURE_NO_WARNINGS
    -DROOT_FOLDER="${PROJECT_SOURCE_DIR}/"
)

set(TARGET_LIBS
    imgui
    glm::glm
    yaml-cpp::yaml-cpp
    lua_script
)

if(WIN32)
    list(APPEND TARGET_LIBS
        glfw
        optick
        loader_assimp
        opengl_4_backend
        d3d11_backend
        d3d12_backend
        vk_backend
    )
elseif (EMSCRIPTEN)
    list(APPEND TARGET_LIBS
        opengles_3_backend
    )
endif()

target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_LIBS})

target_set_warning_level(${TARGET_NAME})

target_precompile_headers(${TARGET_NAME} PRIVATE engine/pch.h)

if(APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
    )
endif()