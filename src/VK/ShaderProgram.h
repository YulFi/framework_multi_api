#pragma once

#include "../RenderAPI/IShaderProgram.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

namespace VK
{
    class Renderer;  // Forward declaration

    struct PushConstantData
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    /**
     * Vulkan implementation of IShaderProgram
     * RAII wrapper for Vulkan shader modules and pipeline
     */
    class ShaderProgram : public IShaderProgram
    {
    public:
        /**
         * Constructor
         * @param name Shader program name
         * @param device Vulkan device
         * @param vertModule Vertex shader module
         * @param fragModule Fragment shader module
         * @param renderer Pointer to renderer (for pipeline creation)
         */
        ShaderProgram(const std::string& name,
                      VkDevice device,
                      VkShaderModule vertModule,
                      VkShaderModule fragModule,
                      Renderer* renderer);

        ~ShaderProgram() override;

        // Delete copy constructor and assignment
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;

        // Move constructor and assignment
        ShaderProgram(ShaderProgram&& other) noexcept;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        // IShaderProgram interface
        void bind() override;
        void unbind() override;

        void setBool(const std::string& name, bool value) override;
        void setInt(const std::string& name, int value) override;
        void setFloat(const std::string& name, float value) override;
        void setVec2(const std::string& name, const glm::vec2& value) override;
        void setVec3(const std::string& name, const glm::vec3& value) override;
        void setVec4(const std::string& name, const glm::vec4& value) override;
        void setMat3(const std::string& name, const glm::mat3& value) override;
        void setMat4(const std::string& name, const glm::mat4& value) override;

        bool isValid() const override { return m_isValid; }
        const std::string& getName() const override { return m_name; }

        // Vulkan-specific methods
        /**
         * Create the Vulkan pipeline for this shader
         * Called by Renderer after shader creation or during swap chain recreation
         */
        void createPipeline(VkRenderPass renderPass,
                           VkPipelineLayout pipelineLayout,
                           VkExtent2D extent);

        /**
         * Destroy the pipeline (for swap chain recreation)
         */
        void destroyPipeline();

        // Vulkan-specific accessors
        VkPipeline getPipeline() const { return m_pipeline; }
        VkShaderModule getVertexModule() const { return m_vertexModule; }
        VkShaderModule getFragmentModule() const { return m_fragmentModule; }

        const PushConstantData& getPushConstants() const { return m_pushConstants; }
        bool hasPendingUpdates() const { return m_hasPendingUpdates; }
        void clearPendingUpdates() { m_hasPendingUpdates = false; }

    private:
        std::string m_name;
        VkDevice m_device;
        VkShaderModule m_vertexModule;
        VkShaderModule m_fragmentModule;
        VkPipeline m_pipeline;
        Renderer* m_renderer;

        PushConstantData m_pushConstants;
        bool m_hasPendingUpdates;
        bool m_isValid;
    };

} // namespace VK
