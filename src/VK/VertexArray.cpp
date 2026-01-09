#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Renderer.h"
#include "../Logger.h"

namespace VK
{

VertexArray::VertexArray(Renderer* renderer)
    : m_renderer(renderer)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_stride(0)
    , m_bindingInitialized(false)
{
    m_binding = {};
    m_binding.binding = 0;
    m_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

VertexArray::~VertexArray()
{
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_renderer(other.m_renderer)
    , m_attributes(std::move(other.m_attributes))
    , m_binding(other.m_binding)
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_indexBuffer(other.m_indexBuffer)
    , m_stride(other.m_stride)
    , m_bindingInitialized(other.m_bindingInitialized)
{
    other.m_renderer = nullptr;
    other.m_vertexBuffer = nullptr;
    other.m_indexBuffer = nullptr;
    other.m_stride = 0;
    other.m_bindingInitialized = false;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other)
    {
        m_renderer = other.m_renderer;
        m_attributes = std::move(other.m_attributes);
        m_binding = other.m_binding;
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_stride = other.m_stride;
        m_bindingInitialized = other.m_bindingInitialized;

        other.m_renderer = nullptr;
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
        other.m_stride = 0;
        other.m_bindingInitialized = false;
    }
    return *this;
}

void VertexArray::bind()
{
    // In Vulkan, binding is done via command buffer
    // Register this VAO as the currently bound one with the renderer
    if (m_renderer)
    {
        m_renderer->setActiveVertexArray(this);
    }
}

void VertexArray::unbind()
{
    // In Vulkan, there's no explicit unbind
    // This is a no-op for compatibility with the interface
}

void VertexArray::addAttribute(const ::VertexAttribute& attribute)
{
    VkVertexInputAttributeDescription vkAttribute{};
    vkAttribute.binding = 0;
    vkAttribute.location = attribute.index;
    vkAttribute.format = getVulkanFormat(attribute.type, attribute.size);
    vkAttribute.offset = reinterpret_cast<uint32_t>(attribute.offset);

    m_attributes.push_back(vkAttribute);

    // Update stride if this is the first attribute or if stride is larger
    if (!m_bindingInitialized)
    {
        m_stride = attribute.stride;
        m_binding.stride = m_stride;
        m_bindingInitialized = true;
    }

    LOG_DEBUG("[Vulkan] VertexArray: Added attribute {} at location {} with offset {}",
              attribute.index, vkAttribute.location, vkAttribute.offset);
}

const std::vector<VkVertexInputAttributeDescription>& VertexArray::getAttributeDescriptions() const
{
    return m_attributes;
}

const VkVertexInputBindingDescription& VertexArray::getBindingDescription() const
{
    return m_binding;
}

void VertexArray::setVertexBuffer(VertexBuffer* buffer)
{
    m_vertexBuffer = buffer;
}

void VertexArray::setIndexBuffer(IndexBuffer* buffer)
{
    m_indexBuffer = buffer;
}

VkFormat VertexArray::getVulkanFormat(DataType type, int size) const
{
    switch (type)
    {
        case DataType::Float:
            switch (size)
            {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                default: return VK_FORMAT_R32_SFLOAT;
            }
        case DataType::Int:
            switch (size)
            {
                case 1: return VK_FORMAT_R32_SINT;
                case 2: return VK_FORMAT_R32G32_SINT;
                case 3: return VK_FORMAT_R32G32B32_SINT;
                case 4: return VK_FORMAT_R32G32B32A32_SINT;
                default: return VK_FORMAT_R32_SINT;
            }
        case DataType::UnsignedInt:
            switch (size)
            {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
                default: return VK_FORMAT_R32_UINT;
            }
        case DataType::Byte:
            switch (size)
            {
                case 1: return VK_FORMAT_R8_SINT;
                case 2: return VK_FORMAT_R8G8_SINT;
                case 3: return VK_FORMAT_R8G8B8_SINT;
                case 4: return VK_FORMAT_R8G8B8A8_SINT;
                default: return VK_FORMAT_R8_SINT;
            }
        case DataType::UnsignedByte:
            switch (size)
            {
                case 1: return VK_FORMAT_R8_UINT;
                case 2: return VK_FORMAT_R8G8_UINT;
                case 3: return VK_FORMAT_R8G8B8_UINT;
                case 4: return VK_FORMAT_R8G8B8A8_UINT;
                default: return VK_FORMAT_R8_UINT;
            }
        default:
            return VK_FORMAT_R32_SFLOAT;
    }
}

} // namespace VK
