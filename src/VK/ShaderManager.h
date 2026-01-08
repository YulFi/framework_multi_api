#pragma once

#include "../RenderAPI/IShaderManager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace VK
{
    class Renderer;  // Forward declaration
    class ShaderProgram;  // Forward declaration

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager() override;

        // Initialize with Vulkan device and renderer
        void initialize(VkDevice device, Renderer* renderer);

        // IShaderManager interface
        IShaderProgram* createShaderProgram(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath) override;

        IShaderProgram* getShader(const std::string& name) override;

        void cleanup() override;

        // Vulkan-specific: Pipeline lifecycle management
        void createAllPipelines(VkRenderPass renderPass,
                               VkPipelineLayout pipelineLayout,
                               VkExtent2D extent);
        void destroyAllPipelines();

        // Vulkan-specific: Get current shader for renderer
        ShaderProgram* getCurrentShader() const { return m_currentShader; }

        // Vulkan-specific: Set current shader (called by ShaderProgram::bind())
        void setCurrentShader(ShaderProgram* shader) { m_currentShader = shader; }

    private:
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readFile(const std::string& filename);

        VkDevice m_device;
        Renderer* m_renderer;

        // Storage for shader programs (ShaderManager owns them)
        std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> m_shaders;

        // Track currently bound shader
        ShaderProgram* m_currentShader;

        std::string m_shaderBasePath;
    };
} // namespace VK
