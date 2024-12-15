@echo off

cd thirdparty\assimp || exit /b 1

if not exist build (
    mkdir build || exit /b 1
)

cd build || exit /b 1
cmake -DBUILD_SHARED_LIBS=OFF .. || exit /b 1
cmake --build . --config Debug || exit /b 1
cmake --build . --config Release || exit /b 1

copy include\assimp\config.h ..\include\assimp\ || exit /b 1
