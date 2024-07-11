#!/bin/bash
glslc -o data/shader/shader.frag -fshader-stage=fragment data/shader.frag.glsl && \
glslc -o data/shader/shader.vert -fshader-stage=vertex data/shader.vert.glsl && \
cmake -S . -B build && cmake --build build
