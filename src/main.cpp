#include <spdlog/common.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>
#include <stdexcept>

#include "Application.h"

int main()
{
    spdlog::logger log("main");
    log.set_level(spdlog::level::debug);
    Application app(log);

    std::cout << "BLA";

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
