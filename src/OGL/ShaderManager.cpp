#include "ShaderManager.h"
#include "ShaderProgram.h"
#include "../Logger.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace OGL
{

ShaderManager::ShaderManager()
    : m_shaderBasePath("shaders/opengl/")
{
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

IShaderProgram* ShaderManager::createShaderProgram(
    const std::string& name,
    const std::string& vertexPath,
    const std::string& fragmentPath)
{
    // Prepend base path
    std::string fullVertexPath = m_shaderBasePath + vertexPath;
    std::string fullFragmentPath = m_shaderBasePath + fragmentPath;

    std::string vertexSource = readFile(fullVertexPath);
    std::string fragmentSource = readFile(fullFragmentPath);

    if (vertexSource.empty())
    {
        LOG_ERROR("Failed to read vertex shader file: '{}'", vertexPath);
        return nullptr;
    }

    if (fragmentSource.empty())
    {
        LOG_ERROR("Failed to read fragment shader file: '{}'", fragmentPath);
        return nullptr;
    }

    if (!loadShader(name, vertexSource, fragmentSource))
    {
        return nullptr;
    }

    return getShader(name);
}

IShaderProgram* ShaderManager::getShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void ShaderManager::cleanup()
{
    // Shader programs are automatically deleted by unique_ptr destructors
    m_shaders.clear();
}

bool ShaderManager::loadShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
{
    GLShader vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader.isValid())
    {
        LOG_ERROR("Failed to compile vertex shader for: '{}'", name);
        return false;
    }

    GLShader fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader.isValid())
    {
        LOG_ERROR("Failed to compile fragment shader for: '{}'", name);
        return false;
    }

    GLShaderProgram program = createProgram(vertexShader, fragmentShader);
    // Shaders are automatically deleted when they go out of scope

    if (!program.isValid())
    {
        LOG_ERROR("Failed to link shader program for: '{}'", name);
        return false;
    }

    GLuint programId = program.get();

    if (m_shaders.find(name) != m_shaders.end())
    {
        LOG_WARNING("Replacing existing shader: '{}'", name);
        // Old shader program will be automatically deleted when unique_ptr is replaced
    }

    // Create ShaderProgram wrapper and store it
    m_shaders[name] = std::make_unique<ShaderProgram>(name, std::move(program));
    LOG_INFO("Shader '{}' loaded successfully (Program ID: {})", name, programId);
    return true;
}

GLShader ShaderManager::compileShader(GLenum type, const std::string& source)
{
    GLuint shaderId = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shaderId, 1, &src, nullptr);
    glCompileShader(shaderId);

    if (!checkCompileErrors(shaderId, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"))
    {
        glDeleteShader(shaderId);
        return GLShader(); // Return invalid shader
    }

    return GLShader(shaderId);
}

GLShaderProgram ShaderManager::createProgram(const GLShader& vertexShader, const GLShader& fragmentShader)
{
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShader.get());
    glAttachShader(programId, fragmentShader.get());
    glLinkProgram(programId);

    if (!checkLinkErrors(programId))
    {
        glDeleteProgram(programId);
        return GLShaderProgram(); // Return invalid program
    }

    return GLShaderProgram(programId);
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
