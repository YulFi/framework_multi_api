#pragma once

#include "IPrimitiveType.h"
#include <glm/glm.hpp>
#include <memory>
#include <cstddef>
#include <string>

struct GLFWwindow;
class IVertexBuffer;
class IVertexArray;
class IIndexBuffer;

// Forward declare OpenGL types to avoid including glad.h
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void initialize() = 0;
    virtual void initialize(GLFWwindow* window) { initialize(); }
    virtual void shutdown() = 0;

    virtual void setClearColor(float r, float g, float b, float a = 1.0f) = 0;
    virtual void setClearColor(const glm::vec4& color) = 0;
    virtual void clear() = 0;

    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void enableDepthTest(bool enable) = 0;
    virtual void enableBlending(bool enable) = 0;
    virtual void enableCulling(bool enable) = 0;

    // Get the actual render surface dimensions (may differ from window size)
    virtual void getRenderDimensions(int& width, int& height) const = 0;

    // Notify renderer that a shader was loaded (for pipeline creation in Vulkan)
    virtual void onShaderLoaded(const std::string& shaderName) {}

    virtual void drawArrays(PrimitiveType mode, int first, int count) = 0;
    virtual void drawElements(PrimitiveType mode, int count, unsigned int indexType, const void* indices) = 0;

    // Factory methods for creating renderer-specific objects
    virtual std::unique_ptr<IVertexBuffer> createVertexBuffer() = 0;
    virtual std::unique_ptr<IVertexArray> createVertexArray() = 0;
    virtual std::unique_ptr<IIndexBuffer> createIndexBuffer() = 0;
};
