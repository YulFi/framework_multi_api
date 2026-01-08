#pragma once

#include "../RenderAPI/IRenderer.h"
#include "../RenderAPI/IVertexBuffer.h"
#include "../RenderAPI/IVertexArray.h"
#include "../RenderAPI/IIndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "ValidationLayers.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

namespace VK
{
    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;

        bool isComplete() const
        {
            return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class Renderer : public IRenderer
    {
    public:
        Renderer();
        ~Renderer() override;

        void initialize() override;
        void initialize(GLFWwindow* window) override;
        void shutdown() override;

        void setClearColor(float r, float g, float b, float a = 1.0f) override;
        void setClearColor(const glm::vec4& color) override;
        void clear() override;

        void setViewport(int x, int y, int width, int height) override;
        void enableDepthTest(bool enable) override;
        void enableBlending(bool enable) override;
        void enableCulling(bool enable) override;

        void getRenderDimensions(int& width, int& height) const override;
        void onShaderLoaded(const std::string& shaderName) override;

        void drawArrays(PrimitiveType mode, int first, int count) override;
        void drawElements(PrimitiveType mode, int count, unsigned int indexType, const void* indices) override;

        void beginFrame();
        void endFrame();

        std::unique_ptr<IVertexBuffer> createVertexBuffer() override;
        std::unique_ptr<IVertexArray> createVertexArray() override;
        std::unique_ptr<IIndexBuffer> createIndexBuffer() override;

        // Vulkan-specific methods
        void setActiveVertexArray(VertexArray* vao);
        VertexArray* getActiveVertexArray() const { return m_boundVertexArray; }
        void setShaderManager(class ShaderManager* shaderManager) { m_shaderManager = shaderManager; }

        // Shader binding (called by ShaderProgram::bind())
        void setCurrentShader(class ShaderProgram* shader) { m_currentShader = shader; }

        // Pipeline management
        VkPipeline createPipelineForShader(VkShaderModule vertModule,
                                           VkShaderModule fragModule,
                                           VkRenderPass renderPass,
                                           VkPipelineLayout pipelineLayout,
                                           VkExtent2D extent);

    private:
        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void initializeVertexBuffer();

        void recreateSwapChain();
        void cleanupSwapChain();

        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readShaderFile(const std::string& filename);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        GLFWwindow* m_window;
        glm::vec4 m_clearColor;

        ValidationLayers m_validationLayers;
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;

        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkRenderPass m_renderPass;
        VkPipelineLayout m_pipelineLayout;
        // m_graphicsPipeline removed - pipelines are now managed by shader manager

        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;

        // Synchronization objects
        // Semaphores: One per swapchain image (to avoid reuse while in flight)
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;

        // Fences: One per frame in flight (to limit CPU-GPU parallelism)
        std::vector<VkFence> m_inFlightFences;

        // Track which fence is waiting on which swapchain image
        std::vector<VkFence> m_imagesInFlight;

        VkBuffer m_vertexBuffer;
        VkDeviceMemory m_vertexBufferMemory;

        // Currently bound vertex array (for user-created vertex data)
        VertexArray* m_boundVertexArray;

        // Shader manager and current shader
        class ShaderManager* m_shaderManager;
        class ShaderProgram* m_currentShader;

        uint32_t m_currentFrame;
        uint32_t m_imageIndex;
        bool m_framebufferResized;
        bool m_frameBegun;

        const int MAX_FRAMES_IN_FLIGHT = 2;
        const std::vector<const char*> m_deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };
} // namespace VK
