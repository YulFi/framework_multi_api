#include "ShaderManager.h"
#include "../Logger.h"
#include <fstream>

namespace VK
{

ShaderManager::ShaderManager()
    : m_device(VK_NULL_HANDLE)
    , m_hasPendingUpdates(false)
{
    // Initialize matrices to identity
    m_pushConstants.model = glm::mat4(1.0f);
    m_pushConstants.view = glm::mat4(1.0f);
    m_pushConstants.projection = glm::mat4(1.0f);
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

void ShaderManager::initialize(VkDevice device)
{
    m_device = device;
    LOG_INFO("[Vulkan] ShaderManager initialized with device");
}

bool ShaderManager::loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    LOG_INFO("[Vulkan] Loading shader '{}'", name);
    LOG_INFO("[Vulkan]   Vertex: {}", vertexPath);
    LOG_INFO("[Vulkan]   Fragment: {}", fragmentPath);

    if (m_device == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] ShaderManager not initialized with device");
        return false;
    }

    // Read SPIR-V files directly
    std::vector<char> vertShaderCode = readFile(vertexPath + ".spv");
    std::vector<char> fragShaderCode = readFile(fragmentPath + ".spv");

    if (vertShaderCode.empty() || fragShaderCode.empty())
    {
        LOG_ERROR("[Vulkan] Failed to read shader files");
        return false;
    }

    // Create shader modules
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] Failed to create shader modules");
        if (vertShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        if (fragShaderModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        return false;
    }

    // Store shader program (pipeline will be created later by renderer)
    ShaderProgram program;
    program.vertexModule = vertShaderModule;
    program.fragmentModule = fragShaderModule;
    program.pipeline = VK_NULL_HANDLE;  // Will be created later
    program.isValid = true;

    m_shaders[name] = program;
    LOG_INFO("[Vulkan] Shader '{}' loaded successfully", name);
    return true;
}

void ShaderManager::use(const std::string& name)
{
    if (m_shaders.find(name) != m_shaders.end())
    {
        m_currentShader = name;
        LOG_DEBUG("[Vulkan] Set current shader: {}", name);
    }
    else
    {
        LOG_ERROR("[Vulkan] Shader '{}' not found", name);
    }
}

void ShaderManager::createPipelineForShader(const std::string& name, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkExtent2D extent)
{
    auto* program = getShaderProgram(name);
    if (!program || !program->isValid)
    {
        LOG_ERROR("[Vulkan] Cannot create pipeline for invalid shader: {}", name);
        return;
    }

    // Pipeline creation will be done by Renderer
    // This method signature exists for future extension
    LOG_INFO("[Vulkan] Pipeline creation requested for shader: {}", name);
}

void ShaderManager::unuse()
{
    m_currentShader.clear();
}

void ShaderManager::setMat4(const std::string& name, const glm::mat4& value)
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
    LOG_DEBUG("[Vulkan] Set mat4 uniform: {}", name);
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

    if (m_device != VK_NULL_HANDLE)
    {
        for (auto& [name, program] : m_shaders)
        {
            if (program.pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_device, program.pipeline, nullptr);
            }
            if (program.vertexModule != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(m_device, program.vertexModule, nullptr);
            }
            if (program.fragmentModule != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(m_device, program.fragmentModule, nullptr);
            }
        }
    }

    m_shaders.clear();
}

VkPipeline ShaderManager::getCurrentPipeline() const
{
    auto it = m_shaders.find(m_currentShader);
    if (it != m_shaders.end() && it->second.isValid)
    {
        return it->second.pipeline;
    }
    return VK_NULL_HANDLE;
}

ShaderProgram* ShaderManager::getShaderProgram(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
        return &it->second;
    }
    return nullptr;
}

VkShaderModule ShaderManager::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        LOG_ERROR("[Vulkan] Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

std::vector<char> ShaderManager::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("[Vulkan] Failed to open file: {}", filename);
        return {};
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    LOG_DEBUG("[Vulkan] Read {} bytes from {}", fileSize, filename);
    return buffer;
}

void ShaderManager::destroyAllPipelines()
{
    if (m_device != VK_NULL_HANDLE)
    {
        for (auto& [name, program] : m_shaders)
        {
            if (program.pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_device, program.pipeline, nullptr);
                program.pipeline = VK_NULL_HANDLE;
            }
        }
        LOG_INFO("[Vulkan] Destroyed all pipelines");
    }
}

std::vector<std::string> ShaderManager::getAllShaderNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, program] : m_shaders)
    {
        names.push_back(name);
    }
    return names;
}

} // namespace VK
