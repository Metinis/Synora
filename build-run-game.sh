#!/bin/bash
cmake -B build -S . && cmake --build build -j$(nproc) && cd build/bin && ./SynoraSandbox
