#!/bin/bash
[[ -d build ]] || mkdir build
/usr/bin/clang++ -g -std=c++17 -Wall -Werror -I. -o build/covid  covid.cpp -lgmock -lgtest -lgtest_main -lpthread
