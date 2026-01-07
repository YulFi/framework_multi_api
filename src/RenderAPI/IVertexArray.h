#pragma once

#include <cstddef>

enum class DataType
{
    Float,
    Int,
    UnsignedInt,
    Byte,
    UnsignedByte
};

struct VertexAttribute
{
    unsigned int index;
    int size;
    DataType type;
    bool normalized;
    size_t stride;
    const void* offset;

    VertexAttribute(unsigned int idx, int sz, DataType t, bool norm, size_t str, const void* off)
        : index(idx), size(sz), type(t), normalized(norm), stride(str), offset(off)
    {
    }
};

class IVertexArray
{
public:
    virtual ~IVertexArray() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void addAttribute(const VertexAttribute& attribute) = 0;
};
