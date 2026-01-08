#include "MyApp.h"
#include "Logger.h"

MyApp::MyApp(const std::string& pluginPath)
    : Application(800, 600, "Professional 3D Renderer - Trackball Camera", pluginPath)
    , m_basicShader(nullptr)
    , m_lastMouseX(0.0)
    , m_lastMouseY(0.0)
    , m_mousePressed(false)
{
}

void MyApp::onInit()
{
    LOG_INFO("MyApp initialized");

    // Create shader program using new API
    m_basicShader = m_shaderManager->createShaderProgram("basic", "basic.vert", "basic.frag");
    if (!m_basicShader || !m_basicShader->isValid())
    {
        LOG_ERROR("Failed to load shaders!");
        return;
    }

    // Notify renderer that shader was loaded (Vulkan needs this to create pipeline)
    onShaderLoaded("basic");

    float vertices[] = {
        // Positions          // Colors
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // Bottom left - Red
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // Bottom right - Green
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f   // Top - Blue
    };

    m_VAO = m_renderer->createVertexArray();
    m_VBO = m_renderer->createVertexBuffer();

    m_VAO->bind();
    m_VBO->setData(vertices, sizeof(vertices), BufferUsage::Static);

    // Position attribute
    m_VAO->addAttribute(VertexAttribute(0, 3, DataType::Float, false, 6 * sizeof(float), (void*)0));

    // Color attribute
    m_VAO->addAttribute(VertexAttribute(1, 3, DataType::Float, false, 6 * sizeof(float), (void*)(3 * sizeof(float))));

    m_VAO->unbind();

    // Set clear color via Application (this ensures consistency across all render APIs)
    // The Application class will forward this to the renderer plugin
    setClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    m_renderer->enableDepthTest(true);

    m_camera->setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    m_camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));

    LOG_INFO("Triangle created and shaders loaded from files");
    LOG_INFO("Controls:");
    LOG_INFO("  - Left Mouse + Drag: Rotate camera (trackball)");
    LOG_INFO("  - Mouse Wheel: Zoom in/out");
    LOG_INFO("  - R: Reset camera");
    LOG_INFO("  - W/A/S/D: Pan camera");
}

void MyApp::onUpdate(float deltaTime)
{
    if (m_window->isKeyPressed(GLFW_KEY_W))
        m_camera->processKeyboard(CameraMovement::UP, deltaTime);
    if (m_window->isKeyPressed(GLFW_KEY_S))
        m_camera->processKeyboard(CameraMovement::DOWN, deltaTime);
    if (m_window->isKeyPressed(GLFW_KEY_A))
        m_camera->processKeyboard(CameraMovement::LEFT, deltaTime);
    if (m_window->isKeyPressed(GLFW_KEY_D))
        m_camera->processKeyboard(CameraMovement::RIGHT, deltaTime);
}

void MyApp::onRender()
{
    if (!m_basicShader)
    {
        LOG_ERROR("Shader not loaded!");
        return;
    }

    // Bind shader using new API
    m_basicShader->bind();

    // Get actual render dimensions from the renderer (may differ from window size)
    int renderWidth, renderHeight;
    m_renderer->getRenderDimensions(renderWidth, renderHeight);
    float aspectRatio = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);

    glm::mat4 projection = m_camera->getProjectionMatrix(aspectRatio);
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    // Set uniforms directly on shader
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("model", model);

    m_VAO->bind();
    m_renderer->drawArrays(PrimitiveType::Triangles, 0, 3);
    m_VAO->unbind();
}

void MyApp::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_mousePressed = true;
            m_window->getCursorPos(m_lastMouseX, m_lastMouseY);
        }
        else if (action == GLFW_RELEASE)
        {
            m_mousePressed = false;
        }
    }
}

void MyApp::onMouseMove(double xpos, double ypos)
{
    if (m_mousePressed)
    {
        glm::vec2 prevPos(m_lastMouseX, m_lastMouseY);
        glm::vec2 currPos(xpos, ypos);
        glm::vec2 screenSize(m_window->getWidth(), m_window->getHeight());

        m_camera->processTrackball(prevPos, currPos, screenSize);

        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
    }
}

void MyApp::onMouseScroll(double xoffset, double yoffset)
{
    m_camera->processMouseScroll(static_cast<float>(yoffset));
}

void MyApp::onKeyPressed(int key, int scancode, int action, int mods)
{
    Application::onKeyPressed(key, scancode, action, mods);

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        m_camera->reset();
        LOG_INFO("Camera reset to initial position");
    }
}

void MyApp::onShutdown()
{
    m_VAO.reset();
    m_VBO.reset();
    m_basicShader = nullptr;  // Shader is owned by ShaderManager, just clear the pointer
    LOG_INFO("MyApp shutting down");
}
