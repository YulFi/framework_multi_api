#include "WindowManager.h"
#include <iostream>

WindowManager::WindowManager(int width, int height, const std::string& title, RenderAPIType apiType)
    : m_window(nullptr)
    , m_width(width)
    , m_height(height)
    , m_title(title)
    , m_apiType(apiType)
{
}

WindowManager::~WindowManager()
{
    shutdown();
}

bool WindowManager::initialize()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    if (m_apiType == RenderAPIType::OpenGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    }
    else if (m_apiType == RenderAPIType::Vulkan)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        std::cout << "Creating window for Vulkan (no OpenGL context)" << std::endl;
    }

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_window, this);

    if (m_apiType == RenderAPIType::OpenGL)
    {
        glfwMakeContextCurrent(m_window);
    }

    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetScrollCallback(m_window, scrollCallback);

    return true;
}

void WindowManager::shutdown()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool WindowManager::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void WindowManager::swapBuffers()
{
    glfwSwapBuffers(m_window);
}

void WindowManager::pollEvents()
{
    glfwPollEvents();
}

void WindowManager::setFramebufferSizeCallback(std::function<void(int, int)> callback)
{
    m_framebufferSizeCallback = callback;
}

void WindowManager::setKeyCallback(std::function<void(int, int, int, int)> callback)
{
    m_keyCallback = callback;
}

void WindowManager::setMouseButtonCallback(std::function<void(int, int, int)> callback)
{
    m_mouseButtonCallback = callback;
}

void WindowManager::setCursorPosCallback(std::function<void(double, double)> callback)
{
    m_cursorPosCallback = callback;
}

void WindowManager::setScrollCallback(std::function<void(double, double)> callback)
{
    m_scrollCallback = callback;
}

bool WindowManager::isKeyPressed(int key) const
{
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool WindowManager::isMouseButtonPressed(int button) const
{
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

void WindowManager::getCursorPos(double& xpos, double& ypos) const
{
    glfwGetCursorPos(m_window, &xpos, &ypos);
}

void WindowManager::setCursorMode(int mode)
{
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void WindowManager::setVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
}

void WindowManager::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    WindowManager* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    manager->m_width = width;
    manager->m_height = height;

    if (manager->m_framebufferSizeCallback)
    {
        manager->m_framebufferSizeCallback(width, height);
    }
}

void WindowManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    WindowManager* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (manager->m_keyCallback)
    {
        manager->m_keyCallback(key, scancode, action, mods);
    }
}

void WindowManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    WindowManager* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (manager->m_mouseButtonCallback)
    {
        manager->m_mouseButtonCallback(button, action, mods);
    }
}

void WindowManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    WindowManager* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (manager->m_cursorPosCallback)
    {
        manager->m_cursorPosCallback(xpos, ypos);
    }
}

void WindowManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowManager* manager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (manager->m_scrollCallback)
    {
        manager->m_scrollCallback(xoffset, yoffset);
    }
}
