#include "ShaderProgram.h"
#include "../Logger.h"
#include <glm/gtc/type_ptr.hpp>

namespace OGL
{

ShaderProgram::ShaderProgram(const std::string& name, GLShaderProgram&& program)
    : m_name(name)
    , m_program(std::move(program))
    , m_isBound(false)
{
}

void ShaderProgram::bind()
{
    if (!m_isBound && m_program.isValid())
    {
        glUseProgram(m_program.get());
        m_isBound = true;
    }
}

void ShaderProgram::unbind()
{
    if (m_isBound)
    {
        glUseProgram(0);
        m_isBound = false;
    }
}

void ShaderProgram::setBool(const std::string& name, bool value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform1i(location, static_cast<int>(value));
    }
}

void ShaderProgram::setInt(const std::string& name, int value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform1i(location, value);
    }
    else
    {
        LOG_WARNING("[OpenGL] Uniform '{}' not found in shader '{}'", name, m_name);
    }
}

void ShaderProgram::setFloat(const std::string& name, float value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform1f(location, value);
    }
}

void ShaderProgram::setVec2(const std::string& name, const glm::vec2& value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::setVec4(const std::string& name, const glm::vec4& value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::setMat3(const std::string& name, const glm::mat3& value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& value)
{
    GLint location = glGetUniformLocation(m_program.get(), name.c_str());
    if (location != -1)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

} // namespace OGL
