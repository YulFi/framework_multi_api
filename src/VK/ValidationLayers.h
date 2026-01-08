#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace VK
{
    /**
     * Vulkan validation layers manager
     * Provides debug callbacks and validation layer setup for development builds
     */
    class ValidationLayers
    {
    public:
        ValidationLayers();
        ~ValidationLayers();

        // Check if validation layers are supported
        bool checkValidationLayerSupport() const;

        // Get required validation layer names
        const std::vector<const char*>& getRequiredLayers() const { return m_validationLayers; }

        // Get required instance extensions for validation
        std::vector<const char*> getRequiredExtensions() const;

        // Setup debug messenger
        void setupDebugMessenger(VkInstance instance);

        // Cleanup debug messenger
        void cleanup(VkInstance instance);

        // Check if validation is enabled
        bool isEnabled() const { return m_enableValidationLayers; }

        // Debug callback
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        // Populate debug messenger create info
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    private:
        bool m_enableValidationLayers;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        std::vector<const char*> m_validationLayers;

        // Extension functions
        VkResult createDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger);

        void destroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator);
    };

} // namespace VK
