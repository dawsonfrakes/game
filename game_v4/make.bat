@echo off
if not exist build/ (mkdir build)
pushd build
glslc ../shaders/texture.vert -o vert.spv
glslc ../shaders/texture.frag -o frag.spv
clang -o game.exe -std=c99 -Wall -Wextra -Werror=vla -pedantic -Og -ggdb -march=native -ffast-math ..\main.c  ..\steam_api64.lib -lvulkan
popd
