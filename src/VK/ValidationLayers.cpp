#include "ValidationLayers.h"
#include "../Logger.h"
#include <cstring>

namespace VK
{

ValidationLayers::ValidationLayers()
    : m_debugMessenger(VK_NULL_HANDLE)
{
    // Enable validation layers in Debug builds
#ifdef NDEBUG
    m_enableValidationLayers = false;
#else
    m_enableValidationLayers = true;
#endif

    // Standard validation layer
    m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    if (m_enableValidationLayers)
    {
        LOG_INFO("[Vulkan] Validation layers enabled");
    }
    else
    {
        LOG_INFO("[Vulkan] Validation layers disabled (Release build)");
    }
}

ValidationLayers::~ValidationLayers()
{
    // Cleanup is done explicitly via cleanup() method
}

bool ValidationLayers::checkValidationLayerSupport() const
{
    if (!m_enableValidationLayers)
    {
        return true; // Not needed in release builds
    }

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check if all required layers are available
    for (const char* layerName : m_validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            LOG_ERROR("[Vulkan] Validation layer not available: {}", layerName);
            return false;
        }
    }

    LOG_INFO("[Vulkan] All required validation layers are available");
    return true;
}

std::vector<const char*> ValidationLayers::getRequiredExtensions() const
{
    std::vector<const char*> extensions;

    if (m_enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void ValidationLayers::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

void ValidationLayers::setupDebugMessenger(VkInstance instance)
{
    if (!m_enableValidationLayers)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
        LOG_ERROR("[Vulkan] Failed to set up debug messenger");
        return;
    }

    LOG_INFO("[Vulkan] Debug messenger created successfully");
}

void ValidationLayers::cleanup(VkInstance instance)
{
    if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE)
    {
        destroyDebugUtilsMessengerEXT(instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
        LOG_INFO("[Vulkan] Debug messenger destroyed");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayers::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // Format message type
    std::string typeStr;
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        typeStr += "GENERAL ";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        typeStr += "VALIDATION ";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        typeStr += "PERFORMANCE ";

    // Log based on severity
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[Vulkan Validation] [{}] {}", typeStr, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARNING("[Vulkan Validation] [{}] {}", typeStr, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LOG_INFO("[Vulkan Validation] [{}] {}", typeStr, pCallbackData->pMessage);
    }
    else
    {
        LOG_DEBUG("[Vulkan Validation] [{}] {}", typeStr, pCallbackData->pMessage);
    }

    // Return VK_FALSE to continue execution
    return VK_FALSE;
}

VkResult ValidationLayers::createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void ValidationLayers::destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

} // namespace VK
