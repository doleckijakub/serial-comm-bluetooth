#!/usr/bin/env bash

set -xe

g++ \
    -o bcp \
    -std=c++20 \
    -Ivendor \
    ./src/main.cpp
