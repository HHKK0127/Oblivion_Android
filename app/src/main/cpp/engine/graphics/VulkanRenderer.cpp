#include "VulkanRenderer.h"
#include "../Engine.h"
#include <android/log.h>
#include <algorithm>
#include <set>

namespace oblivion {

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(Engine* engine, uint32_t width, uint32_t height) {
    engine_ = engine;
    width_ = width;
    height_ = height;

    if (!createSwapChain()) {
        return false;
    }
    if (!createImageViews()) {
        return false;
    }
    if (!createRenderPass()) {
        return false;
    }
    if (!createFramebuffers()) {
        return false;
    }
    if (!createCommandPool()) {
        return false;
    }
    if (!createCommandBuffers()) {
        return false;
    }
    if (!createSyncObjects()) {
        return false;
    }

    return true;
}

void VulkanRenderer::cleanup() {
    VkDevice device = engine_->getDevice();

    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (inFlightFences_.size() > i && inFlightFences_[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences_[i], nullptr);
        }
        if (renderFinishedSemaphores_.size() > i && renderFinishedSemaphores_[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphores_[i], nullptr);
        }
        if (imageAvailableSemaphores_.size() > i && imageAvailableSemaphores_[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores_[i], nullptr);
        }
    }

    vkFreeCommandBuffers(device, commandPool_, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
    vkDestroyCommandPool(device, commandPool_, nullptr);

    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(device, renderPass_, nullptr);

    for (auto imageView : swapChainImageViews_) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain_, nullptr);
}

bool VulkanRenderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(engine_->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = engine_->getSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        engine_->getGraphicsQueueFamily(),
        engine_->getGraphicsQueueFamily()
    };

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(engine_->getDevice(), &createInfo, nullptr, &swapChain_);
    if (result != VK_SUCCESS) {
        return false;
    }

    vkGetSwapchainImagesKHR(engine_->getDevice(), swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(engine_->getDevice(), swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;

    return true;
}

bool VulkanRenderer::createImageViews() {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t i = 0; i < swapChainImages_.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(engine_->getDevice(), &createInfo, nullptr, &swapChainImageViews_[i]);
        if (result != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkResult result = vkCreateRenderPass(engine_->getDevice(), &renderPassInfo, nullptr, &renderPass_);
    if (result != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool VulkanRenderer::createFramebuffers() {
    framebuffers_.resize(swapChainImageViews_.size());

    for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews_[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent_.width;
        framebufferInfo.height = swapChainExtent_.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(engine_->getDevice(), &framebufferInfo, nullptr, &framebuffers_[i]);
        if (result != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = engine_->getGraphicsQueueFamily();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(engine_->getDevice(), &poolInfo, nullptr, &commandPool_);
    if (result != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool VulkanRenderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers_.size();

    VkResult result = vkAllocateCommandBuffers(engine_->getDevice(), &allocInfo, commandBuffers_.data());
    if (result != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(engine_->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(engine_->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(engine_->getDevice(), &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void VulkanRenderer::render() {
    vkWaitForFences(engine_->getDevice(), 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(engine_->getDevice(), swapChain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Handle swap chain recreation
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return;
    }

    vkResetFences(engine_->getDevice(), 1, &inFlightFences_[currentFrame_]);

    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(engine_->getGraphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(engine_->getGraphicsQueue(), &presentInfo);

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = framebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // TODO: Add draw commands here

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

void VulkanRenderer::onResize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    vkDeviceWaitIdle(engine_->getDevice());

    cleanup();

    initialize(engine_, width_, height_);
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, engine_->getSurface(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine_->getSurface(), &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, engine_->getSurface(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine_->getSurface(), &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, engine_->getSurface(), &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {width_, height_};
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

} // namespace oblivion
