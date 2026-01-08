#include "ShaderManager.h"
#include "ShaderProgram.h"
#include "Renderer.h"
#include "../Logger.h"
#include <fstream>

namespace VK
{

ShaderManager::ShaderManager()
    : m_device(VK_NULL_HANDLE)
    , m_renderer(nullptr)
    , m_currentShader(nullptr)
    , m_shaderBasePath("shaders/vulkan/")
{
}

ShaderManager::~ShaderManager()
{
    cleanup();
}

void ShaderManager::initialize(VkDevice device, Renderer* renderer)
{
    m_device = device;
    m_renderer = renderer;
    LOG_INFO("[Vulkan] ShaderManager initialized with device and renderer");
}

IShaderProgram* ShaderManager::createShaderProgram(
    const std::string& name,
    const std::string& vertexPath,
    const std::string& fragmentPath)
{
    LOG_INFO("[Vulkan] Loading shader '{}'", name);
    LOG_INFO("[Vulkan]   Vertex: {}", vertexPath);
    LOG_INFO("[Vulkan]   Fragment: {}", fragmentPath);

    if (m_device == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] ShaderManager not initialized with device");
        return nullptr;
    }

    // Prepend base path and read SPIR-V files directly
    std::string fullVertexPath = m_shaderBasePath + vertexPath + ".spv";
    std::string fullFragmentPath = m_shaderBasePath + fragmentPath + ".spv";

    std::vector<char> vertShaderCode = readFile(fullVertexPath);
    std::vector<char> fragShaderCode = readFile(fullFragmentPath);

    if (vertShaderCode.empty() || fragShaderCode.empty())
    {
        LOG_ERROR("[Vulkan] Failed to read shader files");
        return nullptr;
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
        return nullptr;
    }

    // Create shader program wrapper
    auto shaderProgram = std::make_unique<ShaderProgram>(
        name, m_device, vertShaderModule, fragShaderModule, m_renderer);

    if (m_shaders.find(name) != m_shaders.end())
    {
        LOG_WARNING("[Vulkan] Replacing existing shader: '{}'", name);
    }

    // Store shader program
    ShaderProgram* shaderPtr = shaderProgram.get();
    m_shaders[name] = std::move(shaderProgram);

    LOG_INFO("[Vulkan] Shader '{}' loaded successfully", name);
    return shaderPtr;
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

void ShaderManager::createAllPipelines(VkRenderPass renderPass,
                                       VkPipelineLayout pipelineLayout,
                                       VkExtent2D extent)
{
    LOG_INFO("[Vulkan] Creating pipelines for all shaders");
    for (auto& [name, shader] : m_shaders)
    {
        shader->createPipeline(renderPass, pipelineLayout, extent);
    }
}

void ShaderManager::destroyAllPipelines()
{
    LOG_INFO("[Vulkan] Destroying all pipelines");
    for (auto& [name, shader] : m_shaders)
    {
        shader->destroyPipeline();
    }
}

void ShaderManager::cleanup()
{
    LOG_INFO("[Vulkan] Cleaning up shaders");

    // Shader programs are automatically deleted by unique_ptr destructors
    // This will also destroy shader modules and pipelines
    m_shaders.clear();
    m_currentShader = nullptr;
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

} // namespace VK
