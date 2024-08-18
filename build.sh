#!/bin/bash
glslc -o data/shader/shader.frag.spv -fshader-stage=fragment data/shader.frag.glsl && \
glslc -o data/shader/shader.vert.spv -fshader-stage=vertex data/shader.vert.glsl && \
cmake -S . -B build && cmake --build build
