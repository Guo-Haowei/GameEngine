# GameEngine

Yet another game engine.

## Prerequisites

## Build

```
# required vcpkg packages
glfw3
glad[extensions]
glm
bullet3
stb
yaml-cpp
pkgconf
luajit
tinygltf
luabridge3
```

### Windows
```shell
$ sh scripts/build_assimp.sh
$ mkdir build && cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake ..
$ cmake --build . --config Debug
```

### MacOS
```shell
$ mkdir build && cd build
$ cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=<path/to/vcpkg>/scripts/buildsystems/vcpkg.cmake ..
$ cmake --build . --config Debug
```

## Screenshots

<img src="https://github.com/Guo-Haowei/GameEngine/blob/master/documents/editor.png" width="70%">

<img src="https://github.com/Guo-Haowei/GameEngine/blob/master/documents/breakfast_room.png" width="70%">

## Graphics APIs

API           | Implementation
--------------|----------------------
OpenGL        | Done
Direct3D 11   | Done
Direct3D 12   | Done
Vulkan        | WIP
Metal         | WIP
