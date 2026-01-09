#pragma once

#include "../RenderAPI/IVertexArray.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace VK
{
    class VertexBuffer;
    class IndexBuffer;
    class Renderer;

    class VertexArray : public IVertexArray
    {
    public:
        VertexArray(Renderer* renderer = nullptr);
        ~VertexArray() override;

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&& other) noexcept;
        VertexArray& operator=(VertexArray&& other) noexcept;

        void bind() override;
        void unbind() override;
        void addAttribute(const ::VertexAttribute& attribute) override;

        // Vulkan-specific methods
        const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;
        const VkVertexInputBindingDescription& getBindingDescription() const;
        void setVertexBuffer(VertexBuffer* buffer);
        VertexBuffer* getVertexBuffer() const { return m_vertexBuffer; }
        void setIndexBuffer(IndexBuffer* buffer);
        IndexBuffer* getIndexBuffer() const { return m_indexBuffer; }

    private:
        VkFormat getVulkanFormat(DataType type, int size) const;

        Renderer* m_renderer;
        std::vector<VkVertexInputAttributeDescription> m_attributes;
        VkVertexInputBindingDescription m_binding;
        VertexBuffer* m_vertexBuffer;
        IndexBuffer* m_indexBuffer;
        uint32_t m_stride;
        bool m_bindingInitialized;
    };
}

