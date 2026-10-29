#!/usr/bin/env bash 

cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build 
ctest --test-dir build --output-on-failure
