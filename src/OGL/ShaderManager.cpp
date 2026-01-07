#include "ShaderManager.h"
#include "../Logger.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace OGL
{

ShaderManager::ShaderManager()
    : m_currentProgram(0)
    , m_shaderBasePath("shaders/opengl/")
{
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

bool ShaderManager::loadShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0)
    {
        LOG_ERROR("Failed to compile vertex shader for: '{}'", name);
        return false;
    }

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0)
    {
        LOG_ERROR("Failed to compile fragment shader for: '{}'", name);
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint program = createProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program == 0)
    {
        LOG_ERROR("Failed to link shader program for: '{}'", name);
        return false;
    }

    if (m_shaders.find(name) != m_shaders.end())
    {
        LOG_WARNING("Replacing existing shader: '{}'", name);
        glDeleteProgram(m_shaders[name]);
    }

    m_shaders[name] = program;
    LOG_INFO("Shader '{}' loaded successfully (Program ID: {})", name, program);
    return true;
}

bool ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    // Prepend base path
    std::string fullVertexPath = m_shaderBasePath + vertexPath;
    std::string fullFragmentPath = m_shaderBasePath + fragmentPath;

    std::string vertexSource = readFile(fullVertexPath);
    std::string fragmentSource = readFile(fullFragmentPath);

    if (vertexSource.empty())
    {
        LOG_ERROR("Failed to read vertex shader file: '{}'", vertexPath);
        return false;
    }

    if (fragmentSource.empty())
    {
        LOG_ERROR("Failed to read fragment shader file: '{}'", fragmentPath);
        return false;
    }

    return loadShader(name, vertexSource, fragmentSource);
}

void ShaderManager::use(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        m_currentProgram = it->second;
        glUseProgram(m_currentProgram);
    }
    else
    {
        LOG_ERROR("Shader '{}' not found", name);
    }
}

void ShaderManager::unuse()
{
    m_currentProgram = 0;
    glUseProgram(0);
}

GLuint ShaderManager::getProgram(const std::string& name) const
{
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second : 0;
}

void ShaderManager::setBool(const std::string& name, bool value)
{
    glUniform1i(glGetUniformLocation(m_currentProgram, name.c_str()), static_cast<int>(value));
}

void ShaderManager::setInt(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(m_currentProgram, name.c_str()), value);
}

void ShaderManager::setFloat(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(m_currentProgram, name.c_str()), value);
}

void ShaderManager::setVec2(const std::string& name, const glm::vec2& value)
{
    glUniform2fv(glGetUniformLocation(m_currentProgram, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(m_currentProgram, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setVec4(const std::string& name, const glm::vec4& value)
{
    glUniform4fv(glGetUniformLocation(m_currentProgram, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setMat3(const std::string& name, const glm::mat3& value)
{
    glUniformMatrix3fv(glGetUniformLocation(m_currentProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderManager::setMat4(const std::string& name, const glm::mat4& value)
{
    glUniformMatrix4fv(glGetUniformLocation(m_currentProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderManager::cleanup()
{
    for (auto& pair : m_shaders)
    {
        glDeleteProgram(pair.second);
    }
    m_shaders.clear();
    m_currentProgram = 0;
}

GLuint ShaderManager::compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    if (!checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"))
    {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint ShaderManager::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    if (!checkLinkErrors(program))
    {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool ShaderManager::checkCompileErrors(GLuint shader, const std::string& type)
{
    GLint success;
    GLchar infoLog[1024];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        LOG_ERROR("Shader compilation error ({}):", type);
        LOG_ERROR("{}", infoLog);
        return false;
    }
    return true;
}

bool ShaderManager::checkLinkErrors(GLuint program)
{
    GLint success;
    GLchar infoLog[1024];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        LOG_ERROR("Shader linking error:");
        LOG_ERROR("{}", infoLog);
        return false;
    }
    return true;
}

std::string ShaderManager::readFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open shader file: '{}'", filepath);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    if (content.empty())
    {
        LOG_WARNING("Shader file is empty: '{}'", filepath);
    }

    return content;
}

} // namespace OGL
