#pragma once

#include <cstddef>

enum class BufferUsage
{
    Static,
    Dynamic,
    Stream
};

class IVertexBuffer
{
public:
    virtual ~IVertexBuffer() = default;

    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void setData(const void* data, size_t size, BufferUsage usage) = 0;
    virtual void updateData(const void* data, size_t size, size_t offset = 0) = 0;
};
