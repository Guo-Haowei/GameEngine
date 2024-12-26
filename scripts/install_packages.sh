#!/bin/bash
vcpkg install assimp || exit 1
vcpkg install bullet3 || exit 1
vcpkg install glm || exit 1
vcpkg install glad || exit 1
vcpkg install glfw3 || exit 1
vcpkg install luajit || exit 1
vcpkg install stb || exit 1
vcpkg install yaml-cpp || exit 1
