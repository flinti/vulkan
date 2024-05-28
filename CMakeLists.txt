cmake_minimum_required(VERSION 3.20)
project(vulkan)

set(CMAKE_CXX_STANDARD, "20")
set(CMAKE_CXX_STANDARD_REQUIRED, ON)

link_libraries(fmt glfw vulkan)

file(GLOB vulkan_srcs CONFIGURE_DEPENDS "src/*.h" "src/*.cpp")

add_executable(application ${vulkan_srcs})
