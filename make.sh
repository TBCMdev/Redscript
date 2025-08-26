#!/bin/bash

export CXX=g++

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug 

cmake --build build -j6

./build/rscript "$@" "output"