#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace oblivion {

class Engine;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    bool initialize(Engine* engine, uint32_t width, uint32_t height);
    void cleanup();
    void render();
    void onResize(uint32_t width, uint32_t height);

private:
    bool createSwapChain();
    bool createImageViews();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    Engine* engine_ = nullptr;

    // Swapchain
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    std::vector<VkImageView> swapChainImageViews_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;

    // Rendering
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;

    uint32_t currentFrame_ = 0;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

} // namespace oblivion
