#pragma once

#include "../RenderAPI/IIndexBuffer.h"
#include "../RenderAPI/IVertexBuffer.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace VK
{
    class Renderer;

    class IndexBuffer : public IIndexBuffer
    {
    public:
        IndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer = nullptr);
        ~IndexBuffer() override;

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;

        IndexBuffer(IndexBuffer&& other) noexcept;
        IndexBuffer& operator=(IndexBuffer&& other) noexcept;

        void bind() override;
        void unbind() override;
        void setData(const void* data, size_t count, IndexType type, BufferUsage usage) override;
        void updateData(const void* data, size_t count, size_t offset = 0) override;

        size_t getCount() const override { return m_count; }
        IndexType getIndexType() const override { return m_indexType; }

        VkBuffer getBuffer() const { return m_buffer; }
        VkDeviceSize getSize() const { return m_size; }
        VkIndexType getVkIndexType() const;

    private:
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void cleanup();

        static size_t getIndexSize(IndexType type);
        static VkIndexType toVkIndexType(IndexType type);

        Renderer* m_renderer;
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkBuffer m_buffer;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size;
        size_t m_count;
        IndexType m_indexType;
    };
}
