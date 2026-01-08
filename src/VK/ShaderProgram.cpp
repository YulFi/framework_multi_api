#include "ShaderProgram.h"
#include "Renderer.h"
#include "../Logger.h"

namespace VK
{

ShaderProgram::ShaderProgram(const std::string& name,
                             VkDevice device,
                             VkShaderModule vertModule,
                             VkShaderModule fragModule,
                             Renderer* renderer)
    : m_name(name)
    , m_device(device)
    , m_vertexModule(vertModule)
    , m_fragmentModule(fragModule)
    , m_pipeline(VK_NULL_HANDLE)
    , m_renderer(renderer)
    , m_hasPendingUpdates(false)
    , m_isValid(true)
{
    // Initialize matrices to identity
    m_pushConstants.model = glm::mat4(1.0f);
    m_pushConstants.view = glm::mat4(1.0f);
    m_pushConstants.projection = glm::mat4(1.0f);
}

ShaderProgram::~ShaderProgram()
{
    if (m_device != VK_NULL_HANDLE)
    {
        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
        if (m_vertexModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_vertexModule, nullptr);
        }
        if (m_fragmentModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_fragmentModule, nullptr);
        }
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_device(other.m_device)
    , m_vertexModule(other.m_vertexModule)
    , m_fragmentModule(other.m_fragmentModule)
    , m_pipeline(other.m_pipeline)
    , m_renderer(other.m_renderer)
    , m_pushConstants(other.m_pushConstants)
    , m_hasPendingUpdates(other.m_hasPendingUpdates)
    , m_isValid(other.m_isValid)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_vertexModule = VK_NULL_HANDLE;
    other.m_fragmentModule = VK_NULL_HANDLE;
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_renderer = nullptr;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        // Clean up existing resources
        if (m_device != VK_NULL_HANDLE)
        {
            if (m_pipeline != VK_NULL_HANDLE)
                vkDestroyPipeline(m_device, m_pipeline, nullptr);
            if (m_vertexModule != VK_NULL_HANDLE)
                vkDestroyShaderModule(m_device, m_vertexModule, nullptr);
            if (m_fragmentModule != VK_NULL_HANDLE)
                vkDestroyShaderModule(m_device, m_fragmentModule, nullptr);
        }

        // Move data
        m_name = std::move(other.m_name);
        m_device = other.m_device;
        m_vertexModule = other.m_vertexModule;
        m_fragmentModule = other.m_fragmentModule;
        m_pipeline = other.m_pipeline;
        m_renderer = other.m_renderer;
        m_pushConstants = other.m_pushConstants;
        m_hasPendingUpdates = other.m_hasPendingUpdates;
        m_isValid = other.m_isValid;

        // Nullify other
        other.m_device = VK_NULL_HANDLE;
        other.m_vertexModule = VK_NULL_HANDLE;
        other.m_fragmentModule = VK_NULL_HANDLE;
        other.m_pipeline = VK_NULL_HANDLE;
        other.m_renderer = nullptr;
    }
    return *this;
}

void ShaderProgram::bind()
{
    // For Vulkan, binding is handled by the Renderer during command buffer recording
    // This method just marks this shader as "current" for the renderer
    // The actual vkCmdBindPipeline will be called by the Renderer
    if (m_renderer)
    {
        m_renderer->setCurrentShader(this);
    }
}

void ShaderProgram::unbind()
{
    // For Vulkan, unbinding is implicit when another shader is bound
    if (m_renderer)
    {
        m_renderer->setCurrentShader(nullptr);
    }
}

void ShaderProgram::setBool(const std::string& name, bool value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    LOG_WARNING("[Vulkan] setBool not implemented for shader '{}'", m_name);
}

void ShaderProgram::setInt(const std::string& name, int value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    LOG_WARNING("[Vulkan] setInt not implemented for shader '{}'", m_name);
}

void ShaderProgram::setFloat(const std::string& name, float value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    LOG_WARNING("[Vulkan] setFloat not implemented for shader '{}'", m_name);
}

void ShaderProgram::setVec2(const std::string& name, const glm::vec2& value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    LOG_WARNING("[Vulkan] setVec2 not implemented for shader '{}'", m_name);
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    // Could extend PushConstantData to include vec3 fields
    LOG_WARNING("[Vulkan] setVec3 not implemented for shader '{}'", m_name);
}

void ShaderProgram::setVec4(const std::string& name, const glm::vec4& value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support matrix uniforms via push constants
    LOG_WARNING("[Vulkan] setVec4 not implemented for shader '{}'", m_name);
}

void ShaderProgram::setMat3(const std::string& name, const glm::mat3& value)
{
    // Vulkan doesn't have traditional uniforms like OpenGL
    // For now, we only support mat4 via push constants
    LOG_WARNING("[Vulkan] setMat3 not implemented for shader '{}'", m_name);
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& value)
{
    if (name == "model")
    {
        m_pushConstants.model = value;
        m_hasPendingUpdates = true;
    }
    else if (name == "view")
    {
        m_pushConstants.view = value;
        m_hasPendingUpdates = true;
    }
    else if (name == "projection")
    {
        m_pushConstants.projection = value;
        m_hasPendingUpdates = true;
    }
    else
    {
        LOG_WARNING("[Vulkan] Unknown mat4 uniform '{}' for shader '{}'", name, m_name);
    }
}

void ShaderProgram::createPipeline(VkRenderPass renderPass,
                                   VkPipelineLayout pipelineLayout,
                                   VkExtent2D extent)
{
    if (m_renderer)
    {
        m_pipeline = m_renderer->createPipelineForShader(
            m_vertexModule, m_fragmentModule, renderPass, pipelineLayout, extent);

        if (m_pipeline == VK_NULL_HANDLE)
        {
            LOG_ERROR("[Vulkan] Failed to create pipeline for shader '{}'", m_name);
            m_isValid = false;
        }
        else
        {
            LOG_INFO("[Vulkan] Pipeline created for shader '{}'", m_name);
        }
    }
}

void ShaderProgram::destroyPipeline()
{
    if (m_device != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
        LOG_DEBUG("[Vulkan] Pipeline destroyed for shader '{}'", m_name);
    }
}

} // namespace VK
