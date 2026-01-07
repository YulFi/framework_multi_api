#include "ShaderManager.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

ShaderManager::ShaderManager()
    : m_currentProgram(0)
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
        std::cerr << "Failed to compile vertex shader for: " << name << std::endl;
        return false;
    }

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0)
    {
        std::cerr << "Failed to compile fragment shader for: " << name << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint program = createProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program == 0)
    {
        std::cerr << "Failed to link shader program for: " << name << std::endl;
        return false;
    }

    if (m_shaders.find(name) != m_shaders.end())
    {
        glDeleteProgram(m_shaders[name]);
    }

    m_shaders[name] = program;
    std::cout << "Shader '" << name << "' loaded successfully (Program ID: " << program << ")" << std::endl;
    return true;
}

bool ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSource = readFile(vertexPath);
    std::string fragmentSource = readFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty())
    {
        std::cerr << "Failed to read shader files for: " << name << std::endl;
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
        std::cerr << "Shader '" << name << "' not found" << std::endl;
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
        std::cerr << "Shader compilation error (" << type << "):\n" << infoLog << std::endl;
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
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
        return false;
    }
    return true;
}

std::string ShaderManager::readFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
