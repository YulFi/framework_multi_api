#pragma once

#include "../RenderAPI/IShaderProgram.h"
#include "GLResource.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace OGL
{
    /**
     * OpenGL implementation of IShaderProgram
     * Wraps the GLShaderProgram RAII class and provides the IShaderProgram interface
     */
    class ShaderProgram : public IShaderProgram
    {
    public:
        /**
         * Constructor
         * @param name Shader program name
         * @param program OpenGL shader program (RAII wrapper)
         */
        ShaderProgram(const std::string& name, GLShaderProgram&& program);

        ~ShaderProgram() override = default;

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

        bool isValid() const override { return m_program.isValid(); }
        const std::string& getName() const override { return m_name; }

        // OpenGL-specific accessor
        GLuint getProgramID() const { return m_program.get(); }

    private:
        std::string m_name;
        GLShaderProgram m_program;
        bool m_isBound;
    };

} // namespace OGL
