#pragma once

#include "Application.h"
#include "RenderAPI/IVertexArray.h"
#include "RenderAPI/IVertexBuffer.h"
#include "RenderAPI/IShaderProgram.h"
#include <memory>

class MyApp : public Application
{
public:
    MyApp(const std::string& pluginPath = "plugins/OGLRenderer.dll");
    ~MyApp() = default;

protected:
    void onInit() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    void onMouseButton(int button, int action, int mods) override;
    void onMouseMove(double xpos, double ypos) override;
    void onMouseScroll(double xoffset, double yoffset) override;
    void onKeyPressed(int key, int scancode, int action, int mods) override;
    void onShutdown() override;

private:
    std::unique_ptr<IVertexArray> m_VAO;
    std::unique_ptr<IVertexBuffer> m_VBO;
    IShaderProgram* m_basicShader;  // Non-owning pointer (ShaderManager owns it)
    double m_lastMouseX;
    double m_lastMouseY;
    bool m_mousePressed;
};
