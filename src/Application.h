#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <spdlog/spdlog.h>


class Application
{
public:
	Application(spdlog::logger &log);
	~Application();
	void run();
private:
	spdlog::logger &log;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
};






#endif
