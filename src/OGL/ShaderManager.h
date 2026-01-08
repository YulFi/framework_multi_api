#pragma once

#include "../RenderAPI/IShaderManager.h"
#include "GLResource.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>

namespace OGL
{
    class ShaderProgram;  // Forward declaration

    class ShaderManager : public IShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager();

        // IShaderManager interface
        IShaderProgram* createShaderProgram(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath) override;

        IShaderProgram* getShader(const std::string& name) override;

        void cleanup() override;

    private:
        // Internal helper to load shader from source
        bool loadShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

        // Private helper methods
        GLShader compileShader(GLenum type, const std::string& source);
        GLShaderProgram createProgram(const GLShader& vertexShader, const GLShader& fragmentShader);
        bool checkCompileErrors(GLuint shader, const std::string& type);
        bool checkLinkErrors(GLuint program);
        std::string readFile(const std::string& filepath);

        // Storage for shader programs (ShaderManager owns them)
        std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> m_shaders;
        std::string m_shaderBasePath;
    };
} // namespace OGL
