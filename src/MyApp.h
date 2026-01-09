#pragma once

#include "Application.h"
#include "RenderAPI/ITexture.h"
#include "RenderAPI/IShaderProgram.h"
#include "../Mesh.h"
#include "RenderMesh.h"
#include "../Material.h"
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
    std::shared_ptr<ITexture> m_texture;
    IShaderProgram* m_basicShader;  // Non-owning pointer (ShaderManager owns it)
    std::shared_ptr<Graphics::Mesh> m_triangleMesh;
    std::unique_ptr<Graphics::RenderMesh> m_gpuMesh;
    std::unique_ptr<Graphics::Material> m_material;
    double m_lastMouseX;
    double m_lastMouseY;
    bool m_mousePressed;
};
