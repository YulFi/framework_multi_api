#include "Renderer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "ShaderManager.h"
#include "ShaderProgram.h"
#include "../Logger.h"
#include <stdexcept>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>

namespace VK
{

// Vertex structure matching our triangle data
struct Vertex {
    float pos[3];
    float color[3];
    float texCoord[2];
};

Renderer::Renderer()
    : m_window(nullptr)
    , m_clearColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_instance(VK_NULL_HANDLE)
    , m_surface(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
    , m_device(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_presentQueue(VK_NULL_HANDLE)
    , m_swapChain(VK_NULL_HANDLE)
    , m_renderPass(VK_NULL_HANDLE)
    , m_descriptorSetLayout(VK_NULL_HANDLE)
    , m_pipelineLayout(VK_NULL_HANDLE)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
    , m_transferCommandPool(VK_NULL_HANDLE)
    , m_vertexBuffer(VK_NULL_HANDLE)
    , m_vertexBufferMemory(VK_NULL_HANDLE)
    , m_boundVertexArray(nullptr)
    , m_shaderManager(nullptr)
    , m_currentShader(nullptr)
    , m_currentTexture(nullptr)
    , m_currentFrame(0)
    , m_imageIndex(0)
    , m_framebufferResized(false)
    , m_frameBegun(false)
    , m_cullingEnabled(false)
{
    // Clear color should be set by Application class via setClearColor()
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::initialize()
{
    LOG_WARNING("[Vulkan] Warning: initialize() called without window handle");
    LOG_WARNING("[Vulkan] Please call initialize(GLFWwindow*) instead");
}

void Renderer::initialize(GLFWwindow* window)
{
    m_window = window;

    LOG_INFO("[Vulkan] Initializing Vulkan renderer...");

    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createPipelineLayout();
    createDescriptorPool();
    // Pipeline creation removed - will be created dynamically when shaders are loaded
    createFramebuffers();
    createCommandPool();
    createTransferCommandPool();
    initializeVertexBuffer();
    createCommandBuffers();
    createSyncObjects();

    // Initialize shader manager with device and renderer pointer
    if (m_shaderManager)
    {
        m_shaderManager->initialize(m_device, this);
    }

    LOG_INFO("[Vulkan] Renderer initialized successfully");
}

void Renderer::shutdown()
{
    // Clear any dangling pointers to user-created resources
    m_boundVertexArray = nullptr;

    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);

        cleanupSwapChain();

        if (m_vertexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
            m_vertexBuffer = VK_NULL_HANDLE;
        }

        if (m_vertexBufferMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
            m_vertexBufferMemory = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_renderFinishedSemaphores.size(); i++)
        {
            if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            }
        }
        m_renderFinishedSemaphores.clear();

        for (size_t i = 0; i < m_imageAvailableSemaphores.size(); i++)
        {
            if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            }
        }
        m_imageAvailableSemaphores.clear();

        for (size_t i = 0; i < m_inFlightFences.size(); i++)
        {
            if (m_inFlightFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
            }
        }
        m_inFlightFences.clear();

        cleanupTransferCommandPool();

        if (m_commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }

        // Destroy descriptor resources (not destroyed in cleanupSwapChain)
        if (m_descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        if (m_pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }

        if (m_descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }

        // Clear vectors to prevent double-cleanup
        m_commandBuffers.clear();
        m_swapChainImages.clear();

        // Cleanup memory allocator
        m_memoryAllocator.reset();

        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    // Cleanup validation layers before destroying instance
    if (m_instance != VK_NULL_HANDLE)
    {
        m_validationLayers.cleanup(m_instance);
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    LOG_INFO("[Vulkan] Renderer shutdown complete");
}

void Renderer::createInstance()
{
    // Check validation layer support
    if (m_validationLayers.isEnabled() && !m_validationLayers.checkValidationLayerSupport())
    {
        LOG_WARNING("[Vulkan] Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Get required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Add validation layer extensions
    auto validationExtensions = m_validationLayers.getRequiredExtensions();
    extensions.insert(extensions.end(), validationExtensions.begin(), validationExtensions.end());

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Setup validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_validationLayers.isEnabled())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.getRequiredLayers().size());
        createInfo.ppEnabledLayerNames = m_validationLayers.getRequiredLayers().data();

        // Setup debug messenger for instance creation/destruction
        m_validationLayers.populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    LOG_INFO("[Vulkan] Instance created");

    // Setup debug messenger for runtime validation
    if (m_validationLayers.isEnabled())
    {
        m_validationLayers.setupDebugMessenger(m_instance);
    }
}

void Renderer::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface");
    }

    LOG_INFO("[Vulkan] Surface created");
}

void Renderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    LOG_INFO("[Vulkan] Selected GPU: {}", deviceProperties.deviceName);
}

void Renderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);

    // Initialize memory allocator
    m_memoryAllocator = std::make_unique<MemoryAllocator>(m_device, m_physicalDevice);

    LOG_INFO("[Vulkan] Logical device created");
}

void Renderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    LOG_INFO("[Vulkan] Swap chain created");
}

void Renderer::createImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image views");
        }
    }

    LOG_INFO("[Vulkan] Image views created");
}

void Renderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }

    LOG_INFO("[Vulkan] Render pass created");
}

void Renderer::createDescriptorSetLayout()
{
    // Texture sampler binding
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    LOG_INFO("[Vulkan] Descriptor set layout created");
}

void Renderer::createPipelineLayout()
{
    // Define push constant range for transformation matrices
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) * 3; // model, view, projection matrices

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    LOG_INFO("[Vulkan] Pipeline layout created");
}

void Renderer::createDescriptorPool()
{
    // Create a descriptor pool large enough for texture samplers
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 100; // Allow up to 100 texture samplers

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 100; // Allow up to 100 descriptor sets

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    LOG_INFO("[Vulkan] Descriptor pool created");
}

VkPipeline Renderer::createPipelineForShader(VkShaderModule vertShaderModule,
                                              VkShaderModule fragShaderModule,
                                              VkRenderPass renderPass,
                                              VkPipelineLayout pipelineLayout,
                                              VkExtent2D extent)
{
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] Invalid shader modules for pipeline creation");
        return VK_NULL_HANDLE;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    // Position attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // Color attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    // Texture coordinate attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Use dynamic viewport and scissor to support window resizing
    // This allows us to update viewport without recreating the pipeline
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;  // Will be set dynamically
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;   // Will be set dynamically

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = m_cullingEnabled ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    // Use counter-clockwise to match OpenGL convention (after Y-axis flip)
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Enable dynamic viewport and scissor to support window resizing
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;  // Add dynamic state
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        LOG_ERROR("[Vulkan] Failed to create graphics pipeline");
        return VK_NULL_HANDLE;
    }

    // Don't destroy shader modules - they're owned by the shader manager
    LOG_INFO("[Vulkan] Graphics pipeline created");
    return pipeline;
}

void Renderer::createFramebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {m_swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    LOG_INFO("[Vulkan] Framebuffers created");
}

void Renderer::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool");
    }

    LOG_INFO("[Vulkan] Command pool created");
}

void Renderer::createTransferCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    // Create command pool for transfer operations
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_transferCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create transfer command pool");
    }

    // Pre-allocate command buffers
    m_transferCommandBuffers.resize(TRANSFER_COMMAND_BUFFER_POOL_SIZE);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_transferCommandPool;
    allocInfo.commandBufferCount = 1;

    // Create fences for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first use doesn't wait

    for (uint32_t i = 0; i < TRANSFER_COMMAND_BUFFER_POOL_SIZE; ++i)
    {
        if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_transferCommandBuffers[i].commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate transfer command buffer");
        }

        if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_transferCommandBuffers[i].fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create transfer command buffer fence");
        }

        m_transferCommandBuffers[i].inUse = false;
    }

    LOG_INFO("[Vulkan] Transfer command pool created with {} buffers", TRANSFER_COMMAND_BUFFER_POOL_SIZE);
}

void Renderer::cleanupTransferCommandPool()
{
    if (m_device == VK_NULL_HANDLE)
        return;

    // Wait for all transfers to complete
    for (auto& cmdBuf : m_transferCommandBuffers)
    {
        if (cmdBuf.fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_device, 1, &cmdBuf.fence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(m_device, cmdBuf.fence, nullptr);
            cmdBuf.fence = VK_NULL_HANDLE;
        }
    }

    m_transferCommandBuffers.clear();

    if (m_transferCommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);
        m_transferCommandPool = VK_NULL_HANDLE;
    }

    LOG_DEBUG("[Vulkan] Transfer command pool cleaned up");
}

TransferCommandBuffer* Renderer::acquireTransferCommandBuffer()
{
    // Try to find a free command buffer
    for (auto& cmdBuf : m_transferCommandBuffers)
    {
        if (!cmdBuf.inUse)
        {
            // Wait for fence if it's still in flight from previous use
            vkWaitForFences(m_device, 1, &cmdBuf.fence, VK_TRUE, UINT64_MAX);
            vkResetFences(m_device, 1, &cmdBuf.fence);

            cmdBuf.inUse = true;
            return &cmdBuf;
        }
    }

    // No free buffer, wait for the oldest one
    // In a real implementation, you'd track age more carefully
    for (auto& cmdBuf : m_transferCommandBuffers)
    {
        vkWaitForFences(m_device, 1, &cmdBuf.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &cmdBuf.fence);
        cmdBuf.inUse = true;
        return &cmdBuf;
    }

    // Should never reach here
    throw std::runtime_error("Failed to acquire transfer command buffer");
}

void Renderer::releaseTransferCommandBuffer(TransferCommandBuffer* cmdBuf)
{
    if (cmdBuf)
    {
        cmdBuf->inUse = false;
    }
}

void Renderer::initializeVertexBuffer()
{
    Vertex vertices[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // Bottom left
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Bottom right
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}   // Top
    };

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

    void* data;
    vkMapMemory(m_device, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, (size_t)bufferInfo.size);
    vkUnmapMemory(m_device, m_vertexBufferMemory);

    LOG_INFO("[Vulkan] Vertex buffer created");
}

void Renderer::createCommandBuffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    LOG_INFO("[Vulkan] Command buffers created");
}

void Renderer::createSyncObjects()
{
    // Create semaphores: one per swapchain image (not per frame in flight)
    // This avoids reusing a semaphore that's still in use by the presentation engine
    size_t imageCount = m_swapChainImages.size();
    m_imageAvailableSemaphores.resize(imageCount);
    m_renderFinishedSemaphores.resize(imageCount);

    // Create fences: one per frame in flight (to limit CPU-GPU parallelism)
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    // Initialize images in flight tracker (which fence is using which image)
    m_imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create semaphores for each swapchain image
    for (size_t i = 0; i < imageCount; i++)
    {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphores");
        }
    }

    // Create fences for frame in flight limiting
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fences");
        }
    }

    LOG_INFO("[Vulkan] Sync objects created ({} semaphore pairs, {} fences)", imageCount, MAX_FRAMES_IN_FLIGHT);
}

void Renderer::cleanupSwapChain()
{
    for (auto framebuffer : m_swapChainFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
    m_swapChainFramebuffers.clear();

    for (auto imageView : m_swapChainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
    }
    m_swapChainImageViews.clear();

    if (m_swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }

    // Pipelines are managed by shader manager - no need to destroy here

    // NOTE: We do NOT destroy descriptor pool, pipeline layout, or descriptor set layout here
    // These resources don't depend on swap chain extent/format and destroying them
    // would invalidate all texture descriptor sets
    // They are only destroyed during final shutdown

    if (m_renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void Renderer::recreateSwapChain()
{
    // Handle window minimization - wait until window is restored
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device);

    // Destroy all pipelines before recreating swap chain
    if (m_shaderManager)
    {
        m_shaderManager->destroyAllPipelines();
    }

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    // Don't recreate descriptor set layout, pipeline layout, or descriptor pool
    // They are persistent and don't depend on swap chain
    createFramebuffers();

    // Reset image-in-flight tracking for new swapchain
    m_imagesInFlight.clear();
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

    // Recreate pipelines for all loaded shaders
    if (m_shaderManager)
    {
        m_shaderManager->createAllPipelines(m_renderPass, m_pipelineLayout, m_swapChainExtent);
        LOG_INFO("[Vulkan] Recreated all pipelines after swap chain recreation");
    }
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // Use UNORM (linear) format to match OpenGL's color space behavior
    // SRGB format would apply gamma correction, making colors appear different
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

std::vector<char> Renderer::readShaderFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

void Renderer::beginFrame()
{
    // Use the shader set by ShaderProgram::bind()
    if (!m_currentShader || m_currentShader->getPipeline() == VK_NULL_HANDLE)
    {
        LOG_WARNING("[Vulkan] No valid shader/pipeline bound - skipping frame");
        return;
    }

    ShaderProgram* currentShader = m_currentShader;

    VkPipeline currentPipeline = currentShader->getPipeline();

    // Wait for the current frame's fence (limits frames in flight)
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire next image from swapchain
    // We use currentFrame to index the acquire semaphore for now, but after we know
    // which image we got, we'll use image-indexed semaphores for rendering
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX,
                                             m_imageAvailableSemaphores[m_currentFrame % m_imageAvailableSemaphores.size()],
                                             VK_NULL_HANDLE, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    // Check if this image is already in use by a previous frame
    // If so, wait for that frame's fence to complete
    if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
    }

    // Mark this image as now being used by the current frame
    m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_currentFrame];

    // Only reset the fence right before using it
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[m_imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    VkClearValue clearColor = {{{m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_commandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the pipeline (we already checked it's valid)
    vkCmdBindPipeline(m_commandBuffers[m_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

    // Bind texture descriptor set if a texture is bound
    if (m_currentTexture && m_currentTexture->getDescriptorSet() != VK_NULL_HANDLE)
    {
        VkDescriptorSet descriptorSet = m_currentTexture->getDescriptorSet();
        vkCmdBindDescriptorSets(
            m_commandBuffers[m_currentFrame],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0, // first set
            1, // descriptor set count
            &descriptorSet,
            0, nullptr // dynamic offsets
        );
        //LOG_DEBUG("[Vulkan] Texture descriptor set bound");
    }

    // Set dynamic viewport with Y-axis flip to match OpenGL convention
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (float)m_swapChainExtent.height;
    viewport.width = (float)m_swapChainExtent.width;
    viewport.height = -(float)m_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &viewport);

    // Set dynamic scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainExtent;
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);

    // Push constants for transformation matrices
    if (currentShader && currentShader->hasPendingUpdates())
    {
        const PushConstantData& pushConstants = currentShader->getPushConstants();
        vkCmdPushConstants(
            m_commandBuffers[m_currentFrame],
            m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstantData),
            &pushConstants
        );
        currentShader->clearPendingUpdates();
    }

    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffers[m_currentFrame], 0, 1, vertexBuffers, offsets);

    // Mark that the frame was successfully begun
    m_frameBegun = true;
}

void Renderer::endFrame()
{
    // Only end frame if it was successfully begun
    if (!m_frameBegun)
    {
        return;
    }

    vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);

    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Wait on the imageAvailable semaphore for the acquired image
    // Use currentFrame modulo to cycle through available semaphores
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame % m_imageAvailableSemaphores.size()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    // Signal the renderFinished semaphore indexed by the swapchain image
    // This ensures each image has its own semaphore and avoids reuse while in presentation
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    // Process deferred deletions for resources that are no longer in use
    processDeferredDeletions();

    // Reset frame begun flag
    m_frameBegun = false;
}

void Renderer::setClearColor(float r, float g, float b, float a)
{
    m_clearColor = glm::vec4(r, g, b, a);
}

void Renderer::setClearColor(const glm::vec4& color)
{
    m_clearColor = color;
}

void Renderer::clear()
{
    // Don't begin frame here - let drawArrays/drawElements call it
    // This allows shaders to be bound before beginFrame() is called
}

void Renderer::setViewport(int x, int y, int width, int height)
{
    m_framebufferResized = true;
}

void Renderer::getRenderDimensions(int& width, int& height) const
{
    width = static_cast<int>(m_swapChainExtent.width);
    height = static_cast<int>(m_swapChainExtent.height);
}

void Renderer::enableDepthTest(bool enable)
{
}

void Renderer::enableBlending(bool enable)
{
}

void Renderer::enableCulling(bool enable)
{
    if (m_cullingEnabled == enable)
    {
        return; // No change
    }

    m_cullingEnabled = enable;
    LOG_INFO("[Vulkan] Culling {}", enable ? "enabled" : "disabled");

    // Pipelines are immutable in Vulkan, so we need to recreate them
    // with the new culling state
    if (m_device != VK_NULL_HANDLE && m_shaderManager)
    {
        vkDeviceWaitIdle(m_device);

        // Destroy existing pipelines
        m_shaderManager->destroyAllPipelines();

        // Recreate pipelines with new culling state
        m_shaderManager->createAllPipelines(m_renderPass, m_pipelineLayout, m_swapChainExtent);

        LOG_INFO("[Vulkan] Pipelines recreated with culling state");
    }
}

void Renderer::drawArrays(PrimitiveType mode, int first, int count)
{
    // Begin frame if not already begun
    if (!m_frameBegun)
    {
        beginFrame();
    }

    // Skip if frame wasn't successfully begun (e.g., no pipeline available)
    if (!m_frameBegun)
    {
        return;
    }

    // Vulkan doesn't need the primitive type in draw call - it's specified in pipeline

    // Use the bound vertex array's vertex buffer if available
    if (m_boundVertexArray && m_boundVertexArray->getVertexBuffer())
    {
        VkBuffer vertexBuffer = m_boundVertexArray->getVertexBuffer()->getBuffer();
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_commandBuffers[m_currentFrame], 0, 1, &vertexBuffer, offsets);
    }

    vkCmdDraw(m_commandBuffers[m_currentFrame], count, 1, first, 0);
    endFrame();
}

void Renderer::drawElements(PrimitiveType mode, int count, unsigned int indexType, const void* indices)
{
    // Begin frame if not already begun
    if (!m_frameBegun)
    {
        beginFrame();
    }

    // Skip if frame wasn't successfully begun (e.g., no pipeline available)
    if (!m_frameBegun)
    {
        return;
    }

    // Vulkan doesn't need the primitive type in draw call - it's specified in pipeline
    vkCmdDraw(m_commandBuffers[m_currentFrame], count, 1, 0, 0);
    endFrame();
}

std::unique_ptr<IVertexBuffer> Renderer::createVertexBuffer()
{
    return std::make_unique<VK::VertexBuffer>(m_device, m_physicalDevice, this);
}

std::unique_ptr<IVertexArray> Renderer::createVertexArray()
{
    return std::make_unique<VK::VertexArray>(this);
}

std::unique_ptr<IIndexBuffer> Renderer::createIndexBuffer()
{
    return std::make_unique<VK::IndexBuffer>(m_device, m_physicalDevice, this);
}

std::unique_ptr<ITexture> Renderer::createTexture()
{
    return std::make_unique<VK::Texture>(m_device, m_physicalDevice, this);
}

void Renderer::setActiveVertexArray(VertexArray* vao)
{
    m_boundVertexArray = vao;
}

void Renderer::onShaderLoaded(const std::string& shaderName)
{
    if (!m_shaderManager)
    {
        LOG_ERROR("[Vulkan] Shader manager not set");
        return;
    }

    // Get the shader program (it's now a ShaderProgram wrapper, not a struct)
    IShaderProgram* iShader = m_shaderManager->getShader(shaderName);
    if (!iShader || !iShader->isValid())
    {
        LOG_ERROR("[Vulkan] Invalid shader program: {}", shaderName);
        return;
    }

    // Cast to VK::ShaderProgram to access Vulkan-specific methods
    ShaderProgram* program = static_cast<ShaderProgram*>(iShader);

    // Create pipeline for this shader
    program->createPipeline(m_renderPass, m_pipelineLayout, m_swapChainExtent);
    if (program->getPipeline() != VK_NULL_HANDLE)
    {
        LOG_INFO("[Vulkan] Pipeline created for shader: {}", shaderName);
    }
    else
    {
        LOG_ERROR("[Vulkan] Failed to create pipeline for shader: {}", shaderName);
    }
}

VkCommandBuffer Renderer::beginSingleTimeCommands()
{
    // Acquire a command buffer from the pool
    TransferCommandBuffer* transferCmd = acquireTransferCommandBuffer();

    // Reset and begin recording
    vkResetCommandBuffer(transferCmd->commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(transferCmd->commandBuffer, &beginInfo);

    return transferCmd->commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    // Find the corresponding transfer command buffer to get its fence
    TransferCommandBuffer* transferCmd = nullptr;
    for (auto& cmd : m_transferCommandBuffers)
    {
        if (cmd.commandBuffer == commandBuffer)
        {
            transferCmd = &cmd;
            break;
        }
    }

    if (!transferCmd)
    {
        LOG_ERROR("[Vulkan] Failed to find transfer command buffer for submission");
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit with fence for tracking completion
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, transferCmd->fence);

    // Wait for completion (still synchronous, but uses per-buffer fence)
    vkWaitForFences(m_device, 1, &transferCmd->fence, VK_TRUE, UINT64_MAX);

    // Release back to pool
    releaseTransferCommandBuffer(transferCmd);
}

void Renderer::deferDeleteSampler(VkSampler sampler)
{
    if (sampler == VK_NULL_HANDLE) return;

    DeferredDeletion deletion;
    deletion.type = DeferredDeletion::Type::Sampler;
    deletion.handle = reinterpret_cast<uint64_t>(sampler);
    deletion.frameIndex = m_currentFrame;
    m_deferredDeletions.push_back(deletion);

    LOG_DEBUG("[Vulkan] Sampler queued for deferred deletion");
}

void Renderer::deferDeleteImageView(VkImageView imageView)
{
    if (imageView == VK_NULL_HANDLE) return;

    DeferredDeletion deletion;
    deletion.type = DeferredDeletion::Type::ImageView;
    deletion.handle = reinterpret_cast<uint64_t>(imageView);
    deletion.frameIndex = m_currentFrame;
    m_deferredDeletions.push_back(deletion);

    LOG_DEBUG("[Vulkan] ImageView queued for deferred deletion");
}

void Renderer::deferDeleteImage(VkImage image)
{
    if (image == VK_NULL_HANDLE) return;

    DeferredDeletion deletion;
    deletion.type = DeferredDeletion::Type::Image;
    deletion.handle = reinterpret_cast<uint64_t>(image);
    deletion.frameIndex = m_currentFrame;
    m_deferredDeletions.push_back(deletion);

    LOG_DEBUG("[Vulkan] Image queued for deferred deletion");
}

void Renderer::deferDeleteDeviceMemory(VkDeviceMemory memory)
{
    if (memory == VK_NULL_HANDLE) return;

    DeferredDeletion deletion;
    deletion.type = DeferredDeletion::Type::DeviceMemory;
    deletion.handle = reinterpret_cast<uint64_t>(memory);
    deletion.frameIndex = m_currentFrame;
    m_deferredDeletions.push_back(deletion);

    LOG_DEBUG("[Vulkan] DeviceMemory queued for deferred deletion");
}

void Renderer::deferDeleteBuffer(VkBuffer buffer)
{
    if (buffer == VK_NULL_HANDLE) return;

    DeferredDeletion deletion;
    deletion.type = DeferredDeletion::Type::Buffer;
    deletion.handle = reinterpret_cast<uint64_t>(buffer);
    deletion.frameIndex = m_currentFrame;
    m_deferredDeletions.push_back(deletion);

    LOG_DEBUG("[Vulkan] Buffer queued for deferred deletion");
}

void Renderer::processDeferredDeletions()
{
    // Process deletions that are at least MAX_FRAMES_IN_FLIGHT frames old
    // This ensures the resource is no longer in use by any in-flight frame

    auto it = m_deferredDeletions.begin();
    while (it != m_deferredDeletions.end())
    {
        // Calculate frame distance (handle wraparound)
        uint32_t frameDistance = (m_currentFrame + MAX_FRAMES_IN_FLIGHT - it->frameIndex) % (MAX_FRAMES_IN_FLIGHT * 10);

        if (frameDistance >= MAX_FRAMES_IN_FLIGHT)
        {
            // Safe to delete this resource
            switch (it->type)
            {
                case DeferredDeletion::Type::Sampler:
                    vkDestroySampler(m_device, reinterpret_cast<VkSampler>(it->handle), nullptr);
                    LOG_DEBUG("[Vulkan] Deferred sampler destroyed");
                    break;
                case DeferredDeletion::Type::ImageView:
                    vkDestroyImageView(m_device, reinterpret_cast<VkImageView>(it->handle), nullptr);
                    LOG_DEBUG("[Vulkan] Deferred imageView destroyed");
                    break;
                case DeferredDeletion::Type::Image:
                    vkDestroyImage(m_device, reinterpret_cast<VkImage>(it->handle), nullptr);
                    LOG_DEBUG("[Vulkan] Deferred image destroyed");
                    break;
                case DeferredDeletion::Type::DeviceMemory:
                    vkFreeMemory(m_device, reinterpret_cast<VkDeviceMemory>(it->handle), nullptr);
                    LOG_DEBUG("[Vulkan] Deferred memory freed");
                    break;
                case DeferredDeletion::Type::Buffer:
                    vkDestroyBuffer(m_device, reinterpret_cast<VkBuffer>(it->handle), nullptr);
                    LOG_DEBUG("[Vulkan] Deferred buffer destroyed");
                    break;
            }

            it = m_deferredDeletions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace VK
