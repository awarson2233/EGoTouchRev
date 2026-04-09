#!/bin/bash
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_CROSSCOMPILING_EMULATOR=wine -DCMAKE_EXE_LINKER_FLAGS="-Wl,--allow-multiple-definition -static -static-libgcc -static-libstdc++"
cmake --build .
