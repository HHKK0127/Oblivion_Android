#pragma once

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <memory>
#include <vector>
#include <optional>
#include <set>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace oblivion {

class VulkanRenderer;

struct InitParams {
    jobject androidApp = nullptr;
    ANativeWindow* nativeWindow = nullptr;
};

struct TouchEvent {
    int pointerId;
    float x;
    float y;
    int action;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class Engine {
public:
    Engine();
    ~Engine();

    bool init(const InitParams& params);
    void shutdown();

    void startLoop();
    void stopLoop();
    void pause();
    void resume();
    void update();
    void processInput();

    bool onSurfaceCreated(ANativeWindow* window);
    bool onSurfaceChanged(uint32_t width, uint32_t height);
    void onSurfaceDestroyed();
    void onTouchEvent(float x, float y, int32_t action, int32_t pointerId);
    void queueTouchEvent(int pointerId, float x, float y, int action);

    // Getter
    VkInstance getInstance() const { return instance_; }
    VkSurfaceKHR getSurface() const { return surface_; }
    VkDevice getDevice() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    uint32_t getGraphicsQueueFamily() const { return graphicsQueueFamily_; }

private:
    void gameLoop();

    bool initializeVulkan();
    void cleanupVulkan();

    bool createInstance();
    bool setupDebugMessenger();
    bool checkValidationLayerSupport();
    bool pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool createLogicalDevice();
    bool createSurface();

    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Vulkan state
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

    // Renderer
    std::unique_ptr<VulkanRenderer> renderer_;

    // Application state
    jobject androidApp_ = nullptr;
    ANativeWindow* nativeWindow_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    bool isRunning_ = false;
    bool isPaused_ = false;
    uint32_t graphicsQueueFamily_ = UINT32_MAX;

    // Game loop thread control
    std::thread gameThread_;
    std::atomic<bool> gameLoopRunning_{false};
    std::atomic<bool> gameLoopPaused_{false};
    std::mutex lifecycleMutex_;
    std::condition_variable lifecycleCv_;

    // Input event queue (thread-safe)
    std::queue<TouchEvent> inputQueue_;
    std::mutex inputMutex_;

    bool enableValidationLayers_ = true;
    const std::vector<const char*> validationLayers_ = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

} // namespace oblivion
