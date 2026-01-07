#pragma once

#include "WindowManager.h"
#include "RenderAPI/IShaderManager.h"
#include "RenderAPI/IRenderer.h"
#include "RenderAPI/PluginLoader.h"
#include "RenderAPI/IRenderPlugin.h"
#include "Camera.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

class Application
{
public:
    Application(int width, int height, const std::string& title, const std::string& pluginPath = "");
    virtual ~Application();

    bool initialize();
    void run();
    void shutdown();

    const std::string& getPluginPath() const { return m_pluginPath; }
    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setClearColor(const glm::vec4& color);

protected:
    virtual void onInit() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onRender() {}
    virtual void onShutdown() {}

    virtual void onKeyPressed(int key, int scancode, int action, int mods);
    virtual void onMouseButton(int button, int action, int mods);
    virtual void onMouseMove(double xpos, double ypos);
    virtual void onMouseScroll(double xoffset, double yoffset);
    virtual void onFramebufferResize(int width, int height);

    std::unique_ptr<WindowManager> m_window;
    std::unique_ptr<IShaderManager> m_shaderManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<IRenderer> m_renderer;

    float m_deltaTime;
    float m_lastFrame;

private:
    void updateDeltaTime();
    bool loadRenderPlugin(const std::string& pluginPath);
    RenderAPIType detectAPIType(const std::string& pluginPath);

    std::string m_pluginPath;
    std::unique_ptr<PluginLoader> m_pluginLoader;
    IRenderPlugin* m_plugin;
    bool m_initialized;
    glm::vec4 m_clearColor;
};
