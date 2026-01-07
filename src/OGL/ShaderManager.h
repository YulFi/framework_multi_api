#pragma once

#include "../RenderAPI/IShaderManager.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace OGL
{
    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager();

        bool loadShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
        bool loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) override;

        void use(const std::string& name) override;
        void unuse() override;

        GLuint getProgram(const std::string& name) const;
        GLuint getCurrentProgram() const { return m_currentProgram; }

        void setBool(const std::string& name, bool value);
        void setInt(const std::string& name, int value);
        void setFloat(const std::string& name, float value) override;
        void setVec2(const std::string& name, const glm::vec2& value);
        void setVec3(const std::string& name, const glm::vec3& value) override;
        void setVec4(const std::string& name, const glm::vec4& value);
        void setMat3(const std::string& name, const glm::mat3& value);
        void setMat4(const std::string& name, const glm::mat4& value) override;

        void cleanup() override;

    private:
        GLuint compileShader(GLenum type, const std::string& source);
        GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
        bool checkCompileErrors(GLuint shader, const std::string& type);
        bool checkLinkErrors(GLuint program);
        std::string readFile(const std::string& filepath);

        std::unordered_map<std::string, GLuint> m_shaders;
        GLuint m_currentProgram;
        std::string m_shaderBasePath;
    };
} // namespace OGL
