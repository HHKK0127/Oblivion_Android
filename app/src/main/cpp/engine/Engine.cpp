#include "Engine.h"
#include "graphics/VulkanRenderer.h"
#include <android/log.h>
#include <android/native_window.h>

#define LOG_TAG "OblivionEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

Engine::Engine() = default;

Engine::~Engine() {
    shutdown();
}

bool Engine::init(const InitParams& params) {
    LOGI(">>> Engine::init() STARTING <<<");
    androidApp_ = params.androidApp;
    nativeWindow_ = params.nativeWindow;
    LOGI(">>> Set params (app=%p, window=%p) <<<", androidApp_, nativeWindow_);

    LOGI(">>> About to call initializeVulkan() <<<");
    if (!initializeVulkan()) {
        LOGE("Failed to initialize Vulkan");
        return false;
    }
    LOGI(">>> initializeVulkan() COMPLETED <<<");

    LOGI(">>> Creating VulkanRenderer <<<");
    renderer_ = std::make_unique<VulkanRenderer>();
    LOGI(">>> Initializing VulkanRenderer <<<");
    if (!renderer_->initialize(this, width_, height_)) {
        LOGE("Failed to initialize Vulkan renderer");
        return false;
    }

    isRunning_ = true;
    LOGI(">>> Engine::init() COMPLETED SUCCESSFULLY <<<");
    return true;
}

void Engine::shutdown() {
    stopLoop();

    if (renderer_) {
        renderer_.reset();
    }

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    cleanupVulkan();

    isRunning_ = false;
    LOGI("Engine shutdown complete");
}

void Engine::startLoop() {
    if (!gameLoopRunning_) {
        gameLoopRunning_ = true;
        gameThread_ = std::thread(&Engine::gameLoop, this);
        LOGI("Game loop thread started");
    }
}

void Engine::stopLoop() {
    if (gameLoopRunning_) {
        gameLoopRunning_ = false;
        lifecycleCv_.notify_all();

        if (gameThread_.joinable()) {
            gameThread_.join();
        }

        // GPU 処理完了まで待機（メモリリーク防止）
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
        }

        LOGI("Game loop thread stopped");
    }
}

void Engine::gameLoop() {
    LOGI("Game Loop Thread Started");

    try {
        int frameCount = 0;
        while (gameLoopRunning_) {
            if (gameLoopPaused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }

            try {
                // Process input
                processInput();

                // Update game state
                update();

                // Render frame
                if (renderer_) {
                    renderer_->render();
                }

                frameCount++;
                if (frameCount % 60 == 0) {
                    LOGI("Game loop running, frames: %d", frameCount);
                }
            } catch (const std::exception& e) {
                LOGE("Error in frame iteration: %s", e.what());
                // Continue loop instead of crashing
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    } catch (const std::exception& e) {
        LOGE("Fatal error in game loop: %s", e.what());
    }

    LOGI("Game Loop Thread Stopped");
}

void Engine::run() {
    while (isRunning_) {
        // Process input
        processInput();

        // Update game state
        update();

        // Render frame
        if (renderer_) {
            renderer_->render();
        }
    }
}

void Engine::pause() {
    isPaused_ = true;
    gameLoopPaused_ = true;
    LOGI("Engine paused");
}

void Engine::resume() {
    isPaused_ = false;
    gameLoopPaused_ = false;
    LOGI("Engine resumed");
}

void Engine::update() {
    if (isPaused_) return;

    // TODO: Update game logic here
}

void Engine::processInput() {
    std::queue<TouchEvent> currentFrame;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        std::swap(inputQueue_, currentFrame);
    }

    while (!currentFrame.empty()) {
        const auto& event = currentFrame.front();
        // TODO: Process touch event (camera movement, player action, etc.)
        LOGI("Input Event: pointerId=%d, x=%.1f, y=%.1f, action=%d",
             event.pointerId, event.x, event.y, event.action);
        currentFrame.pop();
    }
}

bool Engine::onSurfaceCreated(ANativeWindow* window) {
    nativeWindow_ = window;

    if (!createSurface()) {
        return false;
    }

    return true;
}

bool Engine::onSurfaceChanged(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (renderer_) {
        renderer_->onResize(width, height);
    }

    return true;
}

void Engine::onSurfaceDestroyed() {
    if (renderer_) {
        renderer_->cleanup();
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    nativeWindow_ = nullptr;
}

void Engine::onTouchEvent(float x, float y, int32_t action, int32_t pointerId) {
    queueTouchEvent(pointerId, x, y, action);
}

void Engine::queueTouchEvent(int pointerId, float x, float y, int action) {
    std::lock_guard<std::mutex> lock(inputMutex_);
    inputQueue_.push({pointerId, x, y, action});
}

bool Engine::initializeVulkan() {
    LOGI("initializeVulkan() starting");

    // Check if validation layers are available
    LOGI("Checking validation layer support");
    if (enableValidationLayers_ && !checkValidationLayerSupport()) {
        LOGI("Validation layers requested but not available, disabling");
        enableValidationLayers_ = false;
    }

    LOGI("Creating Vulkan instance");
    if (!createInstance()) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }
    LOGI("Vulkan instance created successfully");

    LOGI("Setting up debug messenger");
    if (!setupDebugMessenger()) {
        LOGE("Failed to setup debug messenger");
        return false;
    }
    LOGI("Debug messenger setup completed");

    LOGI("Picking physical device");
    if (!pickPhysicalDevice()) {
        LOGE("Failed to pick physical device");
        return false;
    }
    LOGI("Physical device selected");

    LOGI("Creating logical device");
    if (!createLogicalDevice()) {
        LOGE("Failed to create logical device");
        return false;
    }
    LOGI("Logical device created");

    LOGI("initializeVulkan() completed successfully");
    return true;
}

void Engine::cleanupVulkan() {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (enableValidationLayers_ && debugMessenger_ != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance_, debugMessenger_, nullptr);
        }
        debugMessenger_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool Engine::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers_) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            LOGI("Validation layer not found: %s", layerName);
            return false;
        }
    }
    return true;
}

bool Engine::createInstance() {
    LOGI("createInstance() starting");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Oblivion";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Oblivion Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    LOGI("Created VkApplicationInfo");

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Extensions
    LOGI("Getting required extensions");
    std::vector<const char*> extensions = getRequiredExtensions();
    LOGI("Extensions count: %zu", extensions.size());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layers
    if (enableValidationLayers_) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    LOGI("About to call vkCreateInstance...");
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    LOGI("vkCreateInstance returned: %d", result);

    if (result != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance: %d", result);
        return false;
    }

    LOGI("createInstance() completed successfully");
    return true;
}

bool Engine::setupDebugMessenger() {
    if (!enableValidationLayers_) return true;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr) {
        LOGE("Failed to find vkCreateDebugUtilsMessengerEXT");
        return false;
    }

    VkResult result = func(instance_, &createInfo, nullptr, &debugMessenger_);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create debug messenger: %d", result);
        return false;
    }

    return true;
}

void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

bool Engine::pickPhysicalDevice() {
    LOGI("pickPhysicalDevice() starting");

    uint32_t deviceCount = 0;
    LOGI("Calling vkEnumeratePhysicalDevices (count phase)");
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    LOGI("Device count: %u", deviceCount);

    if (deviceCount == 0) {
        LOGE("Failed to find GPUs with Vulkan support");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    LOGI("Calling vkEnumeratePhysicalDevices (data phase)");
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
    LOGI("Physical devices enumerated");

    for (size_t i = 0; i < devices.size(); i++) {
        LOGI("Checking device %zu", i);
        if (isDeviceSuitable(devices[i])) {
            physicalDevice_ = devices[i];
            LOGI("Selected device %zu", i);
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        LOGE("Failed to find a suitable GPU");
        return false;
    }

    LOGI("pickPhysicalDevice() completed successfully");

    return true;
}

bool Engine::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete();
}

QueueFamilyIndices Engine::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool Engine::createLogicalDevice() {
    LOGI("createLogicalDevice() starting");

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
    LOGI("Found queue families");

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions_.data();

    if (enableValidationLayers_) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    LOGI("About to call vkCreateDevice...");
    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    LOGI("vkCreateDevice returned: %d", result);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create logical device: %d", result);
        return false;
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);

    graphicsQueueFamily_ = indices.graphicsFamily.value();

    return true;
}

bool Engine::createSurface() {
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = nativeWindow_;

    VkResult result = vkCreateAndroidSurfaceKHR(instance_, &createInfo, nullptr, &surface_);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create Android surface: %d", result);
        return false;
    }

    return true;
}

std::vector<const char*> Engine::getRequiredExtensions() {
    std::vector<const char*> extensions;

    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

    if (enableValidationLayers_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOGE("Validation layer: %s", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

} // namespace oblivion
