#!/usr/bin/env bash

set -xe

g++ \
    -o term \
    -std=c++20 \
    -Ivendor/PDCurses -Ivendor/stb \
    $(find ./src -name *.cpp) \
    -lcurses
