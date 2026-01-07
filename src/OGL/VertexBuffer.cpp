#include "VertexBuffer.h"

namespace OGL
{
    VertexBuffer::VertexBuffer()
        : m_bufferID(0)
    {
        glGenBuffers(1, &m_bufferID);
    }

    VertexBuffer::~VertexBuffer()
    {
        if (m_bufferID != 0)
        {
            glDeleteBuffers(1, &m_bufferID);
            m_bufferID = 0;
        }
    }

    VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
        : m_bufferID(other.m_bufferID)
    {
        other.m_bufferID = 0;
    }

    VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
    {
        if (this != &other)
        {
            if (m_bufferID != 0)
            {
                glDeleteBuffers(1, &m_bufferID);
            }
            m_bufferID = other.m_bufferID;
            other.m_bufferID = 0;
        }
        return *this;
    }

    void VertexBuffer::bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
    }

    void VertexBuffer::unbind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VertexBuffer::setData(const void* data, size_t size, ::BufferUsage usage)
    {
        bind();
        GLenum glUsage;
        switch (usage)
        {
            case ::BufferUsage::Static:  glUsage = GL_STATIC_DRAW; break;
            case ::BufferUsage::Dynamic: glUsage = GL_DYNAMIC_DRAW; break;
            case ::BufferUsage::Stream:  glUsage = GL_STREAM_DRAW; break;
            default: glUsage = GL_STATIC_DRAW; break;
        }
        glBufferData(GL_ARRAY_BUFFER, size, data, glUsage);
    }

    void VertexBuffer::updateData(const void* data, size_t size, size_t offset)
    {
        bind();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    }
}
