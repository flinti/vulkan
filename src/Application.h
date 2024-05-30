#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "GraphicsPipeline.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "Mesh.h"
#include "Frame.h"
#include "Instance.h"
#include "Device.h"

#include <cstddef>
#include <memory>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan_core.h>


class Application
{
public:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 800;

	Application(spdlog::logger &log, bool enableValidationLayers, uint32_t concurrentFrames);
	~Application();
	void run();
    void setTargetFps(float targetFps);
private:
    void initWindow();
    void initVulkan(bool validationLayers);
    void createLogicalDevice();
    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    void createRenderPassAndSwapChain();
    void createSwapChain(
        const SwapChainSupportDetails &swapChainSupportDetails, 
        const VkSurfaceFormatKHR &chosenSurfaceFormat
    );
    void recreateSwapChain();
    void createCommandPools();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void mainLoop();
    void updateInfoDisplay();
    void draw();
    void cleanup();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
    static void framebufferResized(GLFWwindow* window, int width, int height);

	spdlog::logger &log;

    uint32_t concurrentFrames;
    GLFWwindow *window = nullptr;
    bool paused = false;
    decltype(std::chrono::high_resolution_clock::now()) startedAtTimePoint;
    float targetFps = 60.f;
    float frameRate = 0.f;
    float secondsRunning = 0.f;
    uint64_t frameCounter = 0;
    std::unique_ptr<Instance> instance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::unique_ptr<Device> device;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    uint32_t currentFrameIndex = 0;
    std::vector<std::unique_ptr<Frame>> frames;
    VkCommandPool transferCommandPool = VK_NULL_HANDLE;
    bool needsSwapChainRecreation = false;
    Mesh testingMesh;
};






#endif
