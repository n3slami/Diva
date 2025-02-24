#!/bin/bash

# Clone dependencies
pushd
cd ./bench/scripts
bash setup_includes.sh
popd

# Build WiredTiger
export COMPILER=gcc
export CC=$COMPILER
export LD=$COMPILER
pushd
cd ./bench/wiredtiger/
mkdir build && cd build 
cmake ../CMakeLists.txt
make -j8
popd

# Build Diva
mkdir build && cd build 
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

