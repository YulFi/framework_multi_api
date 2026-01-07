#pragma once

#include "../RenderAPI/IVertexBuffer.h"
#include <glad/glad.h>
#include <vector>

namespace OGL
{
    class VertexBuffer : public IVertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer() override;

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        VertexBuffer(VertexBuffer&& other) noexcept;
        VertexBuffer& operator=(VertexBuffer&& other) noexcept;

        void bind() override;
        void unbind() override;
        void setData(const void* data, size_t size, ::BufferUsage usage) override;
        void updateData(const void* data, size_t size, size_t offset = 0) override;

        template<typename T>
        void setData(const std::vector<T>& data, ::BufferUsage usage = ::BufferUsage::Static)
        {
            setData(data.data(), data.size() * sizeof(T), usage);
        }

        GLuint getID() const { return m_bufferID; }

    private:
        GLuint m_bufferID;
    };
}
