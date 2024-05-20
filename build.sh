#!/bin/bash
cmake -S . -B build && cmake --build build
glslc -o data/compiled/shader.frag data/shader.frag
glslc -o data/compiled/shader.vert data/shader.vert
