add_library(bullet STATIC
	btBulletCollisionAll.cpp
	btBulletDynamicsAll.cpp
	btLinearMathAll.cpp
)

target_include_directories(bullet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(bullet PUBLIC -D__BT_DISABLE_SSE__ -DBT_USE_DOUBLE_PRECISION)

if(MSVC)
	target_compile_options(bullet PRIVATE /W0)
elseif(APPLE)
else()
	message(FATAL_ERROR "platform not supported")
endif()
