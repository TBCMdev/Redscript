#!/bin/bash

export CXX=g++

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

cmake --build build -j

./build/rscript "$@" "output"