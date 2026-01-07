#include "Renderer.h"
#include <iostream>

Renderer::Renderer()
    : m_clearColor(0.2f, 0.3f, 0.3f, 1.0f)
    , m_depthTestEnabled(false)
    , m_blendingEnabled(false)
    , m_cullingEnabled(false)
{
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::initialize()
{
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    enableDepthTest(true);
    std::cout << "Renderer initialized" << std::endl;
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

void Renderer::clear(GLbitfield mask)
{
    glClear(mask);
}

void Renderer::setViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
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

void Renderer::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    glDrawArrays(mode, first, count);
}

void Renderer::drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    glDrawElements(mode, count, type, indices);
}

void Renderer::checkError(const char* location)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error at " << location << ": ";
        switch (error)
        {
            case GL_INVALID_ENUM:
                std::cerr << "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                std::cerr << "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                std::cerr << "GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                std::cerr << "GL_OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                std::cerr << "Unknown error code: " << error;
                break;
        }
        std::cerr << std::endl;
    }
}
