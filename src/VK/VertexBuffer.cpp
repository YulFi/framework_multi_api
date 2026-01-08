#include "VertexBuffer.h"
#include "Renderer.h"
#include "VertexArray.h"
#include "../Logger.h"
#include <cstring>
#include <stdexcept>

namespace VK
{

VertexBuffer::VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer)
    : m_renderer(renderer)
    , m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_buffer(VK_NULL_HANDLE)
    , m_memory(VK_NULL_HANDLE)
    , m_size(0)
{
}

VertexBuffer::~VertexBuffer()
{
    cleanup();
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_renderer(other.m_renderer)
    , m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
{
    other.m_renderer = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
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

        other.m_renderer = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
    }
    return *this;
}

void VertexBuffer::bind()
{
    // In Vulkan, binding is done via vkCmdBindVertexBuffers in the command buffer
    // This method is a no-op for compatibility with the interface
}

void VertexBuffer::unbind()
{
    // In Vulkan, there's no explicit unbind
    // This method is a no-op for compatibility with the interface
}

void VertexBuffer::setData(const void* data, size_t size, ::BufferUsage usage)
{
    cleanup();

    m_size = size;

    // Create buffer with host visible memory for simplicity
    createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Copy data to buffer
    void* mappedData;
    vkMapMemory(m_device, m_memory, 0, size, 0, &mappedData);
    std::memcpy(mappedData, data, size);
    vkUnmapMemory(m_device, m_memory);

    // Associate with the currently bound vertex array (like OpenGL behavior)
    if (m_renderer)
    {
        VertexArray* boundVAO = m_renderer->getActiveVertexArray();
        if (boundVAO)
        {
            boundVAO->setVertexBuffer(this);
            LOG_DEBUG("[Vulkan] VertexBuffer associated with bound VertexArray");
        }
    }

    LOG_DEBUG("[Vulkan] VertexBuffer created with {} bytes", size);
}

void VertexBuffer::updateData(const void* data, size_t size, size_t offset)
{
    if (m_memory == VK_NULL_HANDLE)
    {
        LOG_ERROR("[Vulkan] Cannot update vertex buffer: buffer not initialized");
        return;
    }

    if (offset + size > m_size)
    {
        LOG_ERROR("[Vulkan] Cannot update vertex buffer: data exceeds buffer size");
        return;
    }

    void* mappedData;
    vkMapMemory(m_device, m_memory, offset, size, 0, &mappedData);
    std::memcpy(mappedData, data, size);
    vkUnmapMemory(m_device, m_memory);
}

void VertexBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan vertex buffer");
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
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
}

uint32_t VertexBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

void VertexBuffer::cleanup()
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
    }
}

} // namespace VK
