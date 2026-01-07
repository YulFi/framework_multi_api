#include "ShaderManager.h"
#include "../Logger.h"

namespace VK
{

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

bool ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    LOG_INFO("[Vulkan] Loading shader '{}'", name);
    LOG_INFO("[Vulkan]   Vertex: {}", vertexPath);
    LOG_INFO("[Vulkan]   Fragment: {}", fragmentPath);
    LOG_INFO("[Vulkan] Note: Full Vulkan implementation would:");
    LOG_INFO("[Vulkan]   - Compile GLSL to SPIR-V");
    LOG_INFO("[Vulkan]   - Create VkShaderModule");
    LOG_INFO("[Vulkan]   - Create graphics pipeline");

    m_shaders[name] = true;
    return true;
}

void ShaderManager::use(const std::string& name)
{
    if (m_shaders.find(name) != m_shaders.end())
    {
        m_currentShader = name;
        LOG_DEBUG("[Vulkan] Bind pipeline: {} (stub)", name);
        // In real Vulkan: vkCmdBindPipeline
    }
    else
    {
        LOG_ERROR("[Vulkan] Shader '{}' not found", name);
    }
}

void ShaderManager::unuse()
{
    m_currentShader.clear();
}

void ShaderManager::setMat4(const std::string& name, const glm::mat4& value)
{
    // In real Vulkan: update push constants or uniform buffer
    LOG_DEBUG("[Vulkan] Set mat4 uniform: {} (stub)", name);
}

void ShaderManager::setVec3(const std::string& name, const glm::vec3& value)
{
    // In real Vulkan: update push constants or uniform buffer
    LOG_DEBUG("[Vulkan] Set vec3 uniform: {} (stub)", name);
}

void ShaderManager::setFloat(const std::string& name, float value)
{
    // In real Vulkan: update push constants or uniform buffer
    LOG_DEBUG("[Vulkan] Set float uniform: {} (stub)", name);
}

void ShaderManager::cleanup()
{
    LOG_INFO("[Vulkan] Cleaning up shaders");
    m_shaders.clear();
}

} // namespace VK
