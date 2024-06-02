#include <memory>
#include <string>
#include <set>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Application.h"

int main(int argc, char *argv[])
{
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdout_sink->set_level(spdlog::level::trace);

    spdlog::logger log("main", {stdout_sink});
    log.set_level(spdlog::level::info);

    std::set<std::string> options;

    for (size_t i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            options.insert(argv[i]);
        }
    }
    if (options.size()) {
        log.info("command line options:");
        for (auto &o : options) {
            log.info("\n\t'{}'", o);
        }
    }

    bool singleFrame = false;
    if (options.find("--single") != options.end()) {
        singleFrame = true;
    }

#ifdef NDEBUG
    log.info("Release build.");
#else
    log.info("Debug build");
#endif

    try {
        Application app(log, true, 3, singleFrame);
        app.run();
    } catch (const std::exception& e) {
        log.critical(e.what());
        return 1;
    }

    return 0;
}
