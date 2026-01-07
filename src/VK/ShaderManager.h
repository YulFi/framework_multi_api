#pragma once

#include "../RenderAPI/IShaderManager.h"
#include <string>
#include <unordered_map>

namespace VK
{
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

    private:
        std::unordered_map<std::string, bool> m_shaders;
        std::string m_currentShader;
    };
} // namespace VK
