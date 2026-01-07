#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void initialize();
    void shutdown();

    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setClearColor(const glm::vec4& color);
    void clear(GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    void setViewport(int x, int y, int width, int height);
    void enableDepthTest(bool enable);
    void enableBlending(bool enable);
    void enableCulling(bool enable);
    void setBlendFunc(GLenum sfactor, GLenum dfactor);
    void setCullFace(GLenum mode);
    void setPolygonMode(GLenum face, GLenum mode);

    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);

    static void checkError(const char* location);

private:
    glm::vec4 m_clearColor;
    bool m_depthTestEnabled;
    bool m_blendingEnabled;
    bool m_cullingEnabled;
};
