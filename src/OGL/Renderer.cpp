#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "../Logger.h"
#include <GLFW/glfw3.h>

namespace OGL
{

Renderer::Renderer()
    : m_clearColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_depthTestEnabled(false)
    , m_blendingEnabled(false)
    , m_cullingEnabled(false)
    , m_viewportWidth(800)
    , m_viewportHeight(600)
{
    // Clear color should be set by Application class via setClearColor()
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::initialize()
{
    enableDepthTest(true);
    LOG_INFO("OpenGL Renderer initialized");
}

void Renderer::initialize(GLFWwindow* window)
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Failed to initialize GLAD");
        return;
    }

    LOG_INFO("OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO("GLSL Version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    LOG_INFO("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    initialize();
}

void Renderer::shutdown()
{
}

void Renderer::setClearColor(float r, float g, float b, float a)
{
    m_clearColor = glm::vec4(r, g, b, a);
    glClearColor(r, g, b, a);
}

void Renderer::setClearColor(const glm::vec4& color)
{
    setClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::clear(GLbitfield mask)
{
    glClear(mask);
}

void Renderer::setViewport(int x, int y, int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(x, y, width, height);
}

void Renderer::getRenderDimensions(int& width, int& height) const
{
    width = m_viewportWidth;
    height = m_viewportHeight;
}

void Renderer::enableDepthTest(bool enable)
{
    m_depthTestEnabled = enable;
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::enableBlending(bool enable)
{
    m_blendingEnabled = enable;
    if (enable)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void Renderer::enableCulling(bool enable)
{
    m_cullingEnabled = enable;
    if (enable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::setBlendFunc(GLenum sfactor, GLenum dfactor)
{
    glBlendFunc(sfactor, dfactor);
}

void Renderer::setCullFace(GLenum mode)
{
    glCullFace(mode);
}

void Renderer::setPolygonMode(GLenum face, GLenum mode)
{
    glPolygonMode(face, mode);
}

GLenum toGLPrimitiveType(PrimitiveType type)
{
    switch (type)
    {
        case PrimitiveType::Points:         return GL_POINTS;
        case PrimitiveType::Lines:          return GL_LINES;
        case PrimitiveType::LineStrip:      return GL_LINE_STRIP;
        case PrimitiveType::LineLoop:       return GL_LINE_LOOP;
        case PrimitiveType::Triangles:      return GL_TRIANGLES;
        case PrimitiveType::TriangleStrip:  return GL_TRIANGLE_STRIP;
        case PrimitiveType::TriangleFan:    return GL_TRIANGLE_FAN;
        default:                            return GL_TRIANGLES;
    }
}

void Renderer::drawArrays(PrimitiveType mode, int first, int count)
{
    glDrawArrays(toGLPrimitiveType(mode), first, count);
}

void Renderer::drawElements(PrimitiveType mode, int count, unsigned int indexType, const void* indices)
{
    glDrawElements(toGLPrimitiveType(mode), count, indexType, indices);
}

void Renderer::checkError(const char* location)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::string errorMsg;
        switch (error)
        {
            case GL_INVALID_ENUM:
                errorMsg = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorMsg = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorMsg = "GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                errorMsg = "GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorMsg = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                errorMsg = "Unknown error code: " + std::to_string(error);
                break;
        }
        LOG_ERROR("OpenGL Error at {}: {}", location, errorMsg);
    }
}

std::unique_ptr<IVertexBuffer> Renderer::createVertexBuffer()
{
    return std::make_unique<OGL::VertexBuffer>();
}

std::unique_ptr<IVertexArray> Renderer::createVertexArray()
{
    return std::make_unique<OGL::VertexArray>();
}

std::unique_ptr<IIndexBuffer> Renderer::createIndexBuffer()
{
    return std::make_unique<OGL::IndexBuffer>();
}

std::unique_ptr<ITexture> Renderer::createTexture()
{
    return std::make_unique<OGL::Texture>();
}

} // namespace OGL
