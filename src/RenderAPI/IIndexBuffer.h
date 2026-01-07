#pragma once

#include "IVertexBuffer.h"
#include <cstddef>

enum class IndexType
{
    UnsignedByte,
    UnsignedShort,
    UnsignedInt
};

class IIndexBuffer
{
public:
    virtual ~IIndexBuffer() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void setData(const void* data, size_t count, IndexType type, BufferUsage usage) = 0;
    virtual void updateData(const void* data, size_t count, size_t offset = 0) = 0;

    virtual size_t getCount() const = 0;
    virtual IndexType getIndexType() const = 0;
};
