#!/bin/bash

export COMPILER=gcc
export CC=$COMPILER
export LD=$COMPILER

cd ../wiredtiger/
mkdir build && cd build 
cmake ../ -DCMAKE_C_FLAGS="-Wno-error=unterminated-string-initialization" -DBUILD_TYPE=Release -DENABLE_PYTHON=OFF -DENABLE_TESTS=OFF
make -j8

