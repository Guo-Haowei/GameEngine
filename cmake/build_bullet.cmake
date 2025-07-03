option(USE_BULLET "Enable Bullet Physics Engine" ON)

if (USE_BULLET)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(ENABLE_VHACD OFF CACHE BOOL "" FORCE)
    set(USE_GLUT OFF CACHE BOOL "" FORCE)
    set(BUILD_ENET OFF CACHE BOOL "" FORCE)
    set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/bullet3)

    set_target_properties(Bullet2FileLoader PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Collision PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Common PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Dynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3Geometry PROPERTIES FOLDER thirdparty)
    set_target_properties(Bullet3OpenCL_clew PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletCollision PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletDynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletInverseDynamics PROPERTIES FOLDER thirdparty)
    set_target_properties(BulletSoftBody PROPERTIES FOLDER thirdparty)
    set_target_properties(LinearMath PROPERTIES FOLDER thirdparty)

    set(BULLET_LIBS
        BulletDynamics
        BulletCollision
        LinearMath
        BulletSoftBody
        CACHE INTERNAL "Bullet library targets"
    )
endif()
