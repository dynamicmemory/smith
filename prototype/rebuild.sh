#!/usr/bin/env bash

rm build -rf
cmake -S . -B build 
cmake --build build
