#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <string>

enum class RenderAPIType
{
    OpenGL,
    Vulkan
};

class WindowManager
{
public:
    WindowManager(int width, int height, const std::string& title, RenderAPIType apiType = RenderAPIType::OpenGL);
    ~WindowManager();

    bool initialize();
    void shutdown();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getWindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

    void setFramebufferSizeCallback(std::function<void(int, int)> callback);
    void setKeyCallback(std::function<void(int, int, int, int)> callback);
    void setMouseButtonCallback(std::function<void(int, int, int)> callback);
    void setCursorPosCallback(std::function<void(double, double)> callback);
    void setScrollCallback(std::function<void(double, double)> callback);

    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    void getCursorPos(double& xpos, double& ypos) const;

    void setCursorMode(int mode);
    void setVSync(bool enabled);

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    RenderAPIType m_apiType;

    std::function<void(int, int)> m_framebufferSizeCallback;
    std::function<void(int, int, int, int)> m_keyCallback;
    std::function<void(int, int, int)> m_mouseButtonCallback;
    std::function<void(double, double)> m_cursorPosCallback;
    std::function<void(double, double)> m_scrollCallback;
};
