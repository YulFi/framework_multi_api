#include "MyApp.h"
#include "TextureUtils.h"
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

    // Set the texture sampler to use texture unit 0
    m_basicShader->bind();
    m_basicShader->setInt("textureSampler", 0);
    m_basicShader->unbind();

    // Create triangle mesh using the new Mesh class
    m_triangleMesh = std::make_shared<Graphics::Mesh>();

    //m_triangleMesh = Graphics::MeshFactory::createCube(1.f);

    // Add vertices with positions, colors, and texture coordinates
    m_triangleMesh->addVertex(
        glm::vec3(-0.5f, -0.5f, 0.0f),  // Position
        glm::vec3(1.0f, 0.0f, 0.0f),     // Color: Red
        glm::vec2(0.0f, 0.0f)             // TexCoord
    );
    m_triangleMesh->addVertex(
        glm::vec3(0.5f, -0.5f, 0.0f),    // Position
        glm::vec3(0.0f, 1.0f, 0.0f),     // Color: Green
        glm::vec2(1.0f, 0.0f)             // TexCoord
    );
    m_triangleMesh->addVertex(
        glm::vec3(0.0f, 0.5f, 0.0f),     // Position
        glm::vec3(0.0f, 0.0f, 1.0f),     // Color: Blue
        glm::vec2(0.5f, 1.0f)             // TexCoord
    );

    // Add the triangle (indices)
    m_triangleMesh->addTriangle(0, 1, 2);

    // Compute normals
    m_triangleMesh->computeFlatNormals();

    // Upload mesh to GPU using RenderMesh
    LOG_INFO("Creating RenderMesh with {} vertices, {} indices",
             m_triangleMesh->getVertexCount(), m_triangleMesh->getIndexCount());
    m_gpuMesh = std::make_unique<Graphics::RenderMesh>(*m_triangleMesh, *m_renderer, BufferUsage::Static);
    LOG_INFO("Triangle mesh uploaded to GPU");

    // Create checkerboard texture
    LOG_INFO("Creating checkerboard texture...");
    m_texture = std::shared_ptr<ITexture>(m_renderer->createTexture().release());
    auto checkerData = TextureUtils::createCheckerboard(256);
    LOG_INFO("Setting texture data...");
    m_texture->setData(checkerData.data(), 256, 256, TextureFormat::RGBA);
    m_texture->setFilter(TextureFilter::Nearest, TextureFilter::Nearest);
    m_texture->setWrap(TextureWrap::Repeat, TextureWrap::Repeat);
    LOG_INFO("Checkerboard texture created successfully");

    // Create material with shader and texture
    m_material = std::make_unique<Graphics::Material>(m_basicShader);
    m_material->setTexture("textureSampler", m_texture, 0);  // Bind texture to sampler uniform at unit 0
    LOG_INFO("Material created with shader and texture");

    // Set clear color via Application (this ensures consistency across all render APIs)
    // The Application class will forward this to the renderer plugin
    setClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    m_renderer->enableDepthTest(true);
    m_renderer->enableCulling(false);  // Disable culling to see both sides

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
    static bool firstFrame = true;
    if (firstFrame) {
        LOG_INFO("onRender called - first frame");
        LOG_INFO("m_material: {}, m_gpuMesh: {}", (void*)m_material.get(), (void*)m_gpuMesh.get());
        firstFrame = false;
    }

    if (!m_material || !m_gpuMesh)
    {
        LOG_ERROR("Material or mesh not loaded!");
        return;
    }

    // Get actual render dimensions from the renderer (may differ from window size)
    int renderWidth, renderHeight;
    m_renderer->getRenderDimensions(renderWidth, renderHeight);
    float aspectRatio = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);

    // TEST: Use identity matrices to see if it's a camera issue
    glm::mat4 projection = glm::mat4(1.0f); // m_camera->getProjectionMatrix(aspectRatio);
    glm::mat4 view = glm::mat4(1.0f); // m_camera->getViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    if (firstFrame) {
        LOG_INFO("TEST: Using identity matrices (no camera transform)");
    }

    // Set material properties (uniforms)
    m_material->setProperty("projection", projection);
    m_material->setProperty("view", view);
    m_material->setProperty("model", model);

    // Bind material (activates shader and binds all textures)
    m_material->bind();

    // Draw mesh
    m_gpuMesh->draw();

    // Unbind material
    m_material->unbind();
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
    m_material.reset();
    m_gpuMesh.reset();
    m_triangleMesh.reset();
    m_texture.reset();
    m_basicShader = nullptr;  // Shader is owned by ShaderManager, just clear the pointer
    LOG_INFO("MyApp shutting down");
}
