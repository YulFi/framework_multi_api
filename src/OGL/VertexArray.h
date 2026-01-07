#pragma once

#include "../RenderAPI/IVertexArray.h"
#include <glad/glad.h>
#include <vector>

namespace OGL
{
    enum class DataType
    {
        Float = GL_FLOAT,
        Int = GL_INT,
        UnsignedInt = GL_UNSIGNED_INT,
        Byte = GL_BYTE,
        UnsignedByte = GL_UNSIGNED_BYTE
    };

    struct VertexAttribute
    {
        GLuint index;
        GLint size;
        DataType type;
        GLboolean normalized;
        GLsizei stride;
        const void* offset;

        VertexAttribute(GLuint idx, GLint sz, DataType t, GLboolean norm, GLsizei str, const void* off)
            : index(idx), size(sz), type(t), normalized(norm), stride(str), offset(off) {}
    };

    class VertexArray : public IVertexArray
    {
    public:
        VertexArray();
        ~VertexArray() override;

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&& other) noexcept;
        VertexArray& operator=(VertexArray&& other) noexcept;

        void bind() override;
        void unbind() override;
        void addAttribute(const ::VertexAttribute& attribute) override;

        // OGL-specific methods
        void addAttribute(const OGL::VertexAttribute& attribute);
        void enableAttribute(GLuint index);
        void disableAttribute(GLuint index);

        GLuint getID() const { return m_arrayID; }

    private:
        GLuint m_arrayID;
    };
}
