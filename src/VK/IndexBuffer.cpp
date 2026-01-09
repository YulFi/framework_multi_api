#include "IndexBuffer.h"
#include "Renderer.h"
#include "../Logger.h"
#include <cstring>
#include <stdexcept>

namespace VK
{

IndexBuffer::IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer)
    : m_renderer(renderer)
    , m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_buffer(VK_NULL_HANDLE)
    , m_memory(VK_NULL_HANDLE)
    , m_size(0)
    , m_count(0)
    , m_indexType(IndexType::UnsignedInt)
{
}

IndexBuffer::~IndexBuffer()
{
    cleanup();
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_renderer(other.m_renderer)
    , m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_count(other.m_count)
    , m_indexType(other.m_indexType)
{
    other.m_renderer = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_count = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        m_renderer = other.m_renderer;
        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_count = other.m_count;
        m_indexType = other.m_indexType;

        other.m_renderer = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_count = 0;
    }
    return *this;
}

void IndexBuffer::bind()
{
    // In Vulkan, binding is done via vkCmdBindIndexBuffer in the command buffer
    // This method is a no-op for compatibility with the interface
}

void IndexBuffer::unbind()
{
    // In Vulkan, there's no explicit unbind
    // This method is a no-op for compatibility with the interface
}

void IndexBuffer::setData(const void* data, size_t count, IndexType type, BufferUsage usage)
{
    cleanup();

    m_count = count;
    m_indexType = type;
    m_size = count * getIndexSize(type);

    // Create buffer with host visible memory for simplicity
    createBuffer(m_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy data to buffer
    void* mappedData;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &mappedData);
    std::memcpy(mappedData, data, m_size);
    vkUnmapMemory(m_device, m_memory);

    // Register this index buffer with the currently bound VAO
    if (m_renderer)
    {
        auto* boundVAO = m_renderer->getActiveVertexArray();
        if (boundVAO)
        {
            boundVAO->setIndexBuffer(this);
        }
    }

    LOG_DEBUG("[Vulkan] IndexBuffer created with {} indices ({} bytes)", count, m_size);
}

void IndexBuffer::updateData(const void* data, size_t count, size_t offset)
{
    if (m_memory == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] Cannot update index buffer: buffer not initialized");
        return;
    }

    size_t size = count * getIndexSize(m_indexType);
    size_t byteOffset = offset * getIndexSize(m_indexType);

    if (byteOffset + size > m_size)
    {
        LOG_ERROR("[Vulkan] Cannot update index buffer: data exceeds buffer size");
        return;
    }

    void* mappedData;
    vkMapMemory(m_device, m_memory, byteOffset, size, 0, &mappedData);
    std::memcpy(mappedData, data, size);
    vkUnmapMemory(m_device, m_memory);
}

VkIndexType IndexBuffer::getVkIndexType() const
{
    return toVkIndexType(m_indexType);
}

void IndexBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan index buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
    {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        throw std::runtime_error("Failed to allocate index buffer memory");
    }

    vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
}

uint32_t IndexBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

void IndexBuffer::cleanup()
{
    if (m_buffer != VK_NULL_HANDLE || m_memory != VK_NULL_HANDLE)
    {
        // CRITICAL: Wait for device to finish using this buffer before destroying it
        // This prevents validation errors about destroying in-use resources
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }

        if (m_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        m_size = 0;
        m_count = 0;
    }
}

size_t IndexBuffer::getIndexSize(IndexType type)
{
    switch (type)
    {
        case IndexType::UnsignedByte:  return sizeof(uint8_t);
        case IndexType::UnsignedShort: return sizeof(uint16_t);
        case IndexType::UnsignedInt:   return sizeof(uint32_t);
        default: return sizeof(uint32_t);
    }
}

VkIndexType IndexBuffer::toVkIndexType(IndexType type)
{
    switch (type)
    {
        case IndexType::UnsignedByte:  return VK_INDEX_TYPE_UINT8_EXT;  // Requires extension
        case IndexType::UnsignedShort: return VK_INDEX_TYPE_UINT16;
        case IndexType::UnsignedInt:   return VK_INDEX_TYPE_UINT32;
        default: return VK_INDEX_TYPE_UINT32;
    }
}

} // namespace VK
