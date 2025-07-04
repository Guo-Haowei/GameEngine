# GameEngine

Yet another game engine.

## Prerequisites

### Windows
```shell
$ sh scripts/build_assimp.sh
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```

### WASM
```shell
$ source /path/to/emsdk/emsdk_env.sh
$ mkdir build-emscripten
$ cd build-emscripten
$ emcmake cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXE_LINKER_FLAGS="-sUSE_GLFW=3 -sUSE_WEBGL2=1 -sFULL_ES3=1 -sPTHREAD_POOL_SIZE=16"
$ mingw32-make
$ cd bin && python -m http.server 8080
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
