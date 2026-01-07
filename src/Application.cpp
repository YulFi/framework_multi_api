#include "Application.h"
#include "Logger.h"

Application::Application(int width, int height, const std::string& title, const std::string& pluginPath)
    : m_window(nullptr)
    , m_camera(std::make_unique<Camera>())
    , m_pluginLoader(std::make_unique<PluginLoader>())
    , m_plugin(nullptr)
    , m_deltaTime(0.0f)
    , m_lastFrame(0.0f)
    , m_pluginPath(pluginPath)
    , m_initialized(false)
{
    RenderAPIType apiType = detectAPIType(pluginPath);
    m_window = std::make_unique<WindowManager>(width, height, title, apiType);

    if (!pluginPath.empty())
    {
        loadRenderPlugin(pluginPath);
    }
}

RenderAPIType Application::detectAPIType(const std::string& pluginPath)
{
    if (pluginPath.find("VK") != std::string::npos || pluginPath.find("Vulkan") != std::string::npos)
    {
        return RenderAPIType::Vulkan;
    }
    return RenderAPIType::OpenGL;
}

bool Application::loadRenderPlugin(const std::string& pluginPath)
{
    if (!m_pluginLoader->loadPlugin(pluginPath))
    {
        LOG_ERROR("Failed to load render plugin: {}", pluginPath);
        return false;
    }

    m_plugin = m_pluginLoader->getPlugin();
    if (!m_plugin)
    {
        LOG_ERROR("Plugin loaded but could not get instance");
        return false;
    }

    m_renderer = m_plugin->createRenderer();
    m_shaderManager = m_plugin->createShaderManager();

    if (!m_renderer || !m_shaderManager)
    {
        LOG_ERROR("Plugin failed to create renderer or shader manager");
        return false;
    }

    LOG_INFO("Renderer plugin loaded successfully");
    return true;
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    if (!m_window->initialize())
    {
        LOG_ERROR("Failed to initialize window");
        return false;
    }

    // Pass window handle to renderer (Vulkan needs it, OpenGL ignores it)
    m_renderer->initialize(m_window->getWindow());

    m_window->setFramebufferSizeCallback([this](int width, int height) {
        onFramebufferResize(width, height);
    });

    m_window->setKeyCallback([this](int key, int scancode, int action, int mods) {
        onKeyPressed(key, scancode, action, mods);
    });

    m_window->setMouseButtonCallback([this](int button, int action, int mods) {
        onMouseButton(button, action, mods);
    });

    m_window->setCursorPosCallback([this](double xpos, double ypos) {
        onMouseMove(xpos, ypos);
    });

    m_window->setScrollCallback([this](double xoffset, double yoffset) {
        onMouseScroll(xoffset, yoffset);
    });

    onInit();

    m_initialized = true;
    LOG_INFO("Application initialized successfully");
    return true;
}

void Application::run()
{
    if (!m_initialized)
    {
        LOG_ERROR("Application not initialized. Call initialize() first.");
        return;
    }

    LOG_INFO("Starting main loop...");

    while (!m_window->shouldClose())
    {
        updateDeltaTime();

        onUpdate(m_deltaTime);

        m_renderer->clear();
        onRender();

        m_window->swapBuffers();
        m_window->pollEvents();
    }

    LOG_INFO("Main loop ended");
}

void Application::shutdown()
{
    if (m_initialized)
    {
        onShutdown();

        // Clean up and destroy renderer/shader manager BEFORE the DLL is unloaded
        if (m_shaderManager)
        {
            m_shaderManager->cleanup();
            m_shaderManager.reset();  // Destroy while DLL is still loaded
        }

        if (m_renderer)
        {
            m_renderer->shutdown();
            m_renderer.reset();  // Destroy while DLL is still loaded
        }

        m_window->shutdown();
        m_initialized = false;
        LOG_INFO("Application shut down");
    }
}

void Application::onKeyPressed(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window->getWindow(), true);
    }
}

void Application::onMouseButton(int button, int action, int mods)
{
}

void Application::onMouseMove(double xpos, double ypos)
{
}

void Application::onMouseScroll(double xoffset, double yoffset)
{
}

void Application::onFramebufferResize(int width, int height)
{
    m_renderer->setViewport(0, 0, width, height);
}

void Application::updateDeltaTime()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    m_deltaTime = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
}
