#include "Render/Renderer.hpp"
#include "Math/Vector2.hpp"

#include <cstdint>
#include <spdlog/spdlog.h>

#include <array>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace bl {

Renderer::Renderer(Window& window, RenderDevice& renderDevice, Swapchain& swapchain)
    : m_window(window), m_renderDevice(renderDevice), m_swapchain(swapchain) 
{
    spdlog::info("Creating vulkan renderer.");
    
    /* Create the swapchain based objects and the render pass. */
    createRenderPass();
    recreateSwappable(false);
}

Renderer::~Renderer()
{
    spdlog::info("Destroying vulkan renderer.");

    /* Destroy the renderer objects. */
    destroySwappable();
    vkDestroyRenderPass(m_renderDevice.getDevice(), m_pass, nullptr);   
}

void Renderer::submit(const Submission& submission)
{

}

void Renderer::displayFrame()
{

    /* Acquire the next image from the swapchain. */
    uint32_t index = 0;
    bool wasRecreated = false;
    m_swapchain.acquireNext(VK_NULL_HANDLE, VK_NULL_HANDLE, index, wasRecreated);

    /* If the swapchain was recreated we must destroy and 
        recreate the objects based on the swapchain images. */
    if (wasRecreated) 
    {
        recreateSwappable();
        return;
    }




}

void Renderer::createRenderPass()
{
    m_depthFormat = m_renderDevice.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    const std::array<VkAttachmentDescription, 2> attachmentDescriptions{
        (const VkAttachmentDescription){ // Color Attachment
            .flags = 0,
            .format = m_swapchain.getColorFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        (const VkAttachmentDescription){ // Depth Attachment
            .flags = 0,
            .format = m_depthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
    };

    const VkAttachmentReference colorAttachmentReference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    
    const std::array<VkSubpassDescription, 1> subpassDescriptions{
        (const VkSubpassDescription){
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr
        }
    };

    const std::array<VkSubpassDependency, 0> subpassDependencies{};

    const VkRenderPassCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = attachmentDescriptions.size(),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = subpassDescriptions.size(),
        .pSubpasses = subpassDescriptions.data(),
        .dependencyCount = subpassDependencies.size(),
        .pDependencies = subpassDependencies.data()
    };

    if (vkCreateRenderPass(m_renderDevice.getDevice(), &createInfo, nullptr, &m_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create a vulkan render pass!");
    }
}

void Renderer::createCommandBuffers()
{
    const VkCommandBufferAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_renderDevice.getCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = m_swapchain.getImageCount(),
    };

    m_swapCommandBuffers.resize(m_swapchain.getImageCount());

    if (vkAllocateCommandBuffers(m_renderDevice.getDevice(), &allocateInfo, m_swapCommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not allocate vulkan swapchain command buffers!");
    }
}

void Renderer::createFrameBuffers()
{
    Extent2D extent = m_window.getExtent();

    /* Create the depth image. */
    const uint32_t graphicsFamilyIndex = m_renderDevice.getGraphicsFamilyIndex();

    const VkImageCreateInfo depthImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_depthFormat,
        .extent = VkExtent3D{(uint32_t)extent.width, (uint32_t)extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    vkCreateImage(m_renderDevice.getDevice(), &depthImageCreateInfo, nullptr, &m_depthImage);

    /* The previous objects must be destroyed before this point. */
    m_swapImageViews.clear();
    m_swapFramebuffers.clear();

    for (VkImage image : m_swapchain.getSwapchainImages())
    {
        const VkImageViewCreateInfo imageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_swapchain.getColorFormat(),
            .components = (const VkComponentMapping){
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = (const VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        VkImageView imageView = {};
        if (vkCreateImageView(m_renderDevice.getDevice(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create a vulkan image view for the renderer!");
        }

        const VkFramebufferCreateInfo framebufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = m_pass,
            .attachmentCount = 1,
            .pAttachments = &imageView,
            .width = (uint32_t)extent.width,
            .height = (uint32_t)extent.height,
            .layers = 0,
        };

        VkFramebuffer framebuffer = {};
        if (vkCreateFramebuffer(m_renderDevice.getDevice(), &framebufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create a vulkan framebuffer for the renderer!");
        }

        m_swapImageViews.push_back(imageView);
        m_swapFramebuffers.push_back(framebuffer);
    }
}

void Renderer::createSyncObjects()
{
    const VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    const VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    m_imageAvailableSemaphores.resize(DEFAULT_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(DEFAULT_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(DEFAULT_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < DEFAULT_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_renderDevice.getDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_renderDevice.getDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_renderDevice.getDevice(), &fenceCreateInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Could not create a vulkan frame semaphore!");
        }
    }
}

void Renderer::recreateSwappable(bool destroy)
{
    if (destroy)
        destroySwappable();

    createFrameBuffers();
    createCommandBuffers();
    createSyncObjects();
}

void Renderer::destroySwappable()
{
    /* Destroy the objects used with the swapchain. */
    vkFreeCommandBuffers(m_renderDevice.getDevice(), m_renderDevice.getCommandPool(), m_swapchain.getImageCount(), m_swapCommandBuffers.data());
    
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++)
    {
        vkDestroyFramebuffer(m_renderDevice.getDevice(), m_swapFramebuffers[i], nullptr);
        vkDestroyImageView(m_renderDevice.getDevice(), m_swapImageViews[i], nullptr);
    }

    for (uint32_t i = 0; i < DEFAULT_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_renderDevice.getDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_renderDevice.getDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_renderDevice.getDevice(), m_inFlightFences[i], nullptr);
    }
}

} /* namespace bl */