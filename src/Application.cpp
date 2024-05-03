#include "Application.h"


Application::Application(spdlog::logger &log)
	: log(log)
{
}
Application::~Application()
{
	log.info("running application destructor...");
}


void Application::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void Application::initWindow()
{
	log.info("initializing window...");
}

void Application::initVulkan()
{
	log.info("initializing vulkan...");
}

void Application::mainLoop()
{
	log.info("starting main loop...");
}

void Application::cleanup()
{
	log.info("cleaning up...");
}
