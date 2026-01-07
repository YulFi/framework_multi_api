#pragma once

#include "../RenderAPI/IRenderer.h"
#include "../RenderAPI/IVertexBuffer.h"
#include "../RenderAPI/IVertexArray.h"
#include "../RenderAPI/IIndexBuffer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

namespace OGL
{
    class Renderer : public IRenderer
    {
    public:
        Renderer();
        ~Renderer();

        void initialize() override;
        void initialize(GLFWwindow* window) override;
        void shutdown() override;

        void setClearColor(float r, float g, float b, float a = 1.0f) override;
        void setClearColor(const glm::vec4& color) override;
        void clear() override;
        void clear(GLbitfield mask);

        void setViewport(int x, int y, int width, int height) override;
        void enableDepthTest(bool enable) override;
        void enableBlending(bool enable) override;
        void enableCulling(bool enable) override;
        void setBlendFunc(GLenum sfactor, GLenum dfactor);
        void setCullFace(GLenum mode);
        void setPolygonMode(GLenum face, GLenum mode);

        void getRenderDimensions(int& width, int& height) const override;

        void drawArrays(PrimitiveType mode, int first, int count) override;
        void drawElements(PrimitiveType mode, int count, unsigned int indexType, const void* indices) override;

        std::unique_ptr<IVertexBuffer> createVertexBuffer() override;
        std::unique_ptr<IVertexArray> createVertexArray() override;
        std::unique_ptr<IIndexBuffer> createIndexBuffer() override;

        static void checkError(const char* location);

    private:
        glm::vec4 m_clearColor;
        bool m_depthTestEnabled;
        bool m_blendingEnabled;
        bool m_cullingEnabled;
        int m_viewportWidth;
        int m_viewportHeight;
    };
}
