#!/usr/bin/env bash

set -xe

g++ \
    -o bcp \
    -std=c++20 \
    -Ivendor/stb \
    ./src/main.cpp
