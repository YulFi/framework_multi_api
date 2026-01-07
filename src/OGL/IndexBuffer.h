#pragma once

#include "../RenderAPI/IIndexBuffer.h"
#include "../RenderAPI/IVertexBuffer.h"
#include <glad/glad.h>
#include <vector>

namespace OGL
{
    class IndexBuffer : public IIndexBuffer
    {
    public:
        IndexBuffer();
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

        // Convenience methods for vectors
        template<typename T>
        void setData(const std::vector<T>& data, IndexType type, BufferUsage usage)
        {
            setData(data.data(), data.size(), type, usage);
        }

        GLuint getID() const { return m_bufferID; }
        GLenum getGLIndexType() const;

    private:
        GLuint m_bufferID;
        size_t m_count;
        IndexType m_indexType;

        static GLenum toGLUsage(BufferUsage usage);
        static GLenum toGLIndexType(IndexType type);
        static size_t getIndexSize(IndexType type);
    };
}
