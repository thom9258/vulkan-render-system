#!/bin/bash

conan build . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
cd ..
ln -s ./build/build/Release/compile_commands.json .
