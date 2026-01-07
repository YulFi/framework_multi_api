#pragma once

#include "../RenderAPI/IVertexBuffer.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace VK
{
    class Renderer;

    class VertexBuffer : public IVertexBuffer
    {
    public:
        VertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer = nullptr);
        ~VertexBuffer() override;

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;

        void bind() override;
        void unbind() override;
        void setData(const void* data, size_t size, ::BufferUsage usage) override;
        void updateData(const void* data, size_t size, size_t offset = 0) override;

        VkBuffer getBuffer() const { return m_buffer; }
        VkDeviceSize getSize() const { return m_size; }

    private:
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void cleanup();

        Renderer* m_renderer;
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkBuffer m_buffer;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size;
    };
}

