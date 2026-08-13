#!/bin/bash
cmake -B build -S . && cmake --build build && cd build/bin && ./SynoraEditor
