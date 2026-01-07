#pragma once

#include "../RenderAPI/IShaderManager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace VK
{
    struct PushConstantData
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct ShaderProgram
    {
        VkShaderModule vertexModule;
        VkShaderModule fragmentModule;
        VkPipeline pipeline;
        bool isValid;
    };

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager() override;

        // Initialize with Vulkan device
        void initialize(VkDevice device);

        bool loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) override;
        void use(const std::string& name) override;
        void unuse() override;

        void setMat4(const std::string& name, const glm::mat4& value) override;
        void setVec3(const std::string& name, const glm::vec3& value) override;
        void setFloat(const std::string& name, float value) override;

        void cleanup() override;

        // Vulkan-specific: Get push constant data
        const PushConstantData& getPushConstantData() const { return m_pushConstants; }
        bool hasPendingUpdates() const { return m_hasPendingUpdates; }
        void clearPendingUpdates() { m_hasPendingUpdates = false; }

        // Vulkan-specific: Pipeline management
        VkPipeline getCurrentPipeline() const;
        ShaderProgram* getShaderProgram(const std::string& name);
        void createPipelineForShader(const std::string& name, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent);
        void destroyAllPipelines();
        std::vector<std::string> getAllShaderNames() const;

    private:
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readFile(const std::string& filename);

        VkDevice m_device;
        std::unordered_map<std::string, ShaderProgram> m_shaders;
        std::string m_currentShader;
        PushConstantData m_pushConstants;
        bool m_hasPendingUpdates;
    };
} // namespace VK
