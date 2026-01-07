#include "VertexArray.h"

namespace OGL
{
    VertexArray::VertexArray()
        : m_arrayID(0)
    {
        glGenVertexArrays(1, &m_arrayID);
    }

    VertexArray::~VertexArray()
    {
        if (m_arrayID != 0)
        {
            glDeleteVertexArrays(1, &m_arrayID);
            m_arrayID = 0;
        }
    }

    VertexArray::VertexArray(VertexArray&& other) noexcept
        : m_arrayID(other.m_arrayID)
    {
        other.m_arrayID = 0;
    }

    VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
    {
        if (this != &other)
        {
            if (m_arrayID != 0)
            {
                glDeleteVertexArrays(1, &m_arrayID);
            }
            m_arrayID = other.m_arrayID;
            other.m_arrayID = 0;
        }
        return *this;
    }

    void VertexArray::bind()
    {
        glBindVertexArray(m_arrayID);
    }

    void VertexArray::unbind()
    {
        glBindVertexArray(0);
    }

    void VertexArray::addAttribute(const ::VertexAttribute& attribute)
    {
        OGL::DataType oglType;
        switch (attribute.type)
        {
            case ::DataType::Float: oglType = OGL::DataType::Float; break;
            case ::DataType::Int: oglType = OGL::DataType::Int; break;
            case ::DataType::UnsignedInt: oglType = OGL::DataType::UnsignedInt; break;
            case ::DataType::Byte: oglType = OGL::DataType::Byte; break;
            case ::DataType::UnsignedByte: oglType = OGL::DataType::UnsignedByte; break;
        }

        OGL::VertexAttribute oglAttr(
            attribute.index,
            attribute.size,
            oglType,
            attribute.normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLsizei>(attribute.stride),
            attribute.offset
        );

        addAttribute(oglAttr);
    }

    void VertexArray::addAttribute(const OGL::VertexAttribute& attribute)
    {
        bind();
        glVertexAttribPointer(
            attribute.index,
            attribute.size,
            static_cast<GLenum>(attribute.type),
            attribute.normalized,
            attribute.stride,
            attribute.offset
        );
        enableAttribute(attribute.index);
    }

    void VertexArray::enableAttribute(GLuint index)
    {
        bind();
        glEnableVertexAttribArray(index);
    }

    void VertexArray::disableAttribute(GLuint index)
    {
        bind();
        glDisableVertexAttribArray(index);
    }
}
