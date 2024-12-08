# GameEngine

Yet another game engine.

## Prerequisites

## Build

### Windows
```shell
$ sh build_assimp.bat
$ mkdir build && cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake ..
$ cmake --build . --config Debug
```

### MacOS
```shell
$ sh build_assimp.sh
$ mkdir build && cd build
$ cmake -G Xcode ..
$ cmake --build . --config Debug
```

## Screenshots

<img src="https://github.com/Guo-Haowei/GameEngine/blob/master/resources/images/screenshots/editor.png" width="70%">

<img src="https://github.com/Guo-Haowei/GameEngine/blob/master/resources/images/screenshots/path_tracer.png" width="70%">

## Graphics APIs

API           | Implementation
--------------|----------------------
OpenGL        | Done
Direct3D 11   | Done
Direct3D 12   | Done
Vulkan        | WIP
Metal         | WIP
