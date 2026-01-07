#pragma once

#include "../RenderAPI/IShaderManager.h"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace VK
{
    struct PushConstantData
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager() override;

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

    private:
        std::unordered_map<std::string, bool> m_shaders;
        std::string m_currentShader;
        PushConstantData m_pushConstants;
        bool m_hasPendingUpdates;
    };
} // namespace VK
