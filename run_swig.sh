#!/bin/bash
swig3.0 -python covid.i
g++ -fPIC -c -isystem/usr/include/python3.6m -std=c++17 -fno-use-cxa-atexit covid_wrapper.cpp covid_wrap.c covid.cpp
ld -shared /usr/lib/x86_64-linux-gnu/libpython3.6m.so /usr/lib/x86_64-linux-gnu/libstdc++.so.6 covid.o covid_wrapper.o covid_wrap.o -o _covid.so

