cmake_minimum_required(VERSION 3.20)
project(vulkan)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD, "20")
set(CMAKE_CXX_STANDARD_REQUIRED, ON)

add_compile_definitions(GLFW_INCLUDE_VULKAN)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

link_libraries(fmt glfw vulkan)

file(GLOB vulkan_srcs CONFIGURE_DEPENDS "src/*.h" "src/*.cpp")

add_executable(application ${vulkan_srcs})

target_precompile_headers(application
    PUBLIC
    <vulkan/vulkan_core.h>
    <spdlog/spdlog.h>
    <spdlog/fmt/fmt.h>
)
