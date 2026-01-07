#include "IndexBuffer.h"

namespace OGL
{
    IndexBuffer::IndexBuffer()
        : m_bufferID(0)
        , m_count(0)
        , m_indexType(IndexType::UnsignedInt)
    {
        glGenBuffers(1, &m_bufferID);
    }

    IndexBuffer::~IndexBuffer()
    {
        if (m_bufferID != 0)
        {
            glDeleteBuffers(1, &m_bufferID);
            m_bufferID = 0;
        }
    }

    IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
        : m_bufferID(other.m_bufferID)
        , m_count(other.m_count)
        , m_indexType(other.m_indexType)
    {
        other.m_bufferID = 0;
        other.m_count = 0;
    }

    IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
    {
        if (this != &other)
        {
            if (m_bufferID != 0)
            {
                glDeleteBuffers(1, &m_bufferID);
            }
            m_bufferID = other.m_bufferID;
            m_count = other.m_count;
            m_indexType = other.m_indexType;
            other.m_bufferID = 0;
            other.m_count = 0;
        }
        return *this;
    }

    void IndexBuffer::bind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
    }

    void IndexBuffer::unbind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void IndexBuffer::setData(const void* data, size_t count, IndexType type, BufferUsage usage)
    {
        m_count = count;
        m_indexType = type;

        bind();
        size_t size = count * getIndexSize(type);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, toGLUsage(usage));
    }

    void IndexBuffer::updateData(const void* data, size_t count, size_t offset)
    {
        bind();
        size_t size = count * getIndexSize(m_indexType);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * getIndexSize(m_indexType), size, data);
    }

    GLenum IndexBuffer::getGLIndexType() const
    {
        return toGLIndexType(m_indexType);
    }

    GLenum IndexBuffer::toGLUsage(BufferUsage usage)
    {
        switch (usage)
        {
            case BufferUsage::Static:  return GL_STATIC_DRAW;
            case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
            case BufferUsage::Stream:  return GL_STREAM_DRAW;
            default: return GL_STATIC_DRAW;
        }
    }

    GLenum IndexBuffer::toGLIndexType(IndexType type)
    {
        switch (type)
        {
            case IndexType::UnsignedByte:  return GL_UNSIGNED_BYTE;
            case IndexType::UnsignedShort: return GL_UNSIGNED_SHORT;
            case IndexType::UnsignedInt:   return GL_UNSIGNED_INT;
            default: return GL_UNSIGNED_INT;
        }
    }

    size_t IndexBuffer::getIndexSize(IndexType type)
    {
        switch (type)
        {
            case IndexType::UnsignedByte:  return sizeof(unsigned char);
            case IndexType::UnsignedShort: return sizeof(unsigned short);
            case IndexType::UnsignedInt:   return sizeof(unsigned int);
            default: return sizeof(unsigned int);
        }
    }
}
