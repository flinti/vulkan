#include <memory>
#include <spdlog/common.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Application.h"

int main()
{
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdout_sink->set_level(spdlog::level::trace);

    spdlog::logger log("main", {stdout_sink});
    log.set_level(spdlog::level::info);

#ifdef NDEBUG
    log.info("Release build.");
#else
    log.info("Debug build");
#endif

    Application app(log, true);

    try {
        app.run();
    } catch (const std::exception& e) {
        log.critical(e.what());
        return 1;
    }

    return 0;
}
