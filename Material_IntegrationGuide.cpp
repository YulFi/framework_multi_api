/**
 * @file Material_IntegrationGuide.cpp
 * @brief Step-by-step integration guide for the Material system
 *
 * This file shows exactly how to integrate the Material class with your
 * existing graphics engine, including:
 * - Interface requirements
 * - ShaderManager integration
 * - TextureManager integration
 * - Renderer integration
 * - Complete working examples
 */

#include "Material.h"
#include <memory>
#include <vector>
#include <unordered_map>

// ================================================================================
// STEP 1: Ensure Your IShaderProgram Interface Matches
// ================================================================================

/**
 * Your existing IShaderProgram interface should have these methods.
 * If it doesn't, add them or create an adapter.
 */
class IShaderProgram {
public:
    virtual ~IShaderProgram() = default;

    // Required by Material system
    virtual void bind() = 0;
    virtual void unbind() = 0;

    virtual void setInt(const std::string& name, int value) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;
    virtual void setVec2(const std::string& name, const glm::vec2& value) = 0;
    virtual void setVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void setVec4(const std::string& name, const glm::vec4& value) = 0;
    virtual void setMat3(const std::string& name, const glm::mat3& value) = 0;
    virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
};

/**
 * Example OpenGL implementation (you likely already have this)
 */
class OpenGLShaderProgram : public IShaderProgram {
    unsigned int m_programID;

public:
    void bind() override {
        glUseProgram(m_programID);
    }

    void unbind() override {
        glUseProgram(0);
    }

    void setInt(const std::string& name, int value) override {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform1i(location, value);
        }
    }

    void setFloat(const std::string& name, float value) override {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform1f(location, value);
        }
    }

    void setVec3(const std::string& name, const glm::vec3& value) override {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniform3fv(location, 1, &value[0]);
        }
    }

    void setMat4(const std::string& name, const glm::mat4& value) override {
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location != -1) {
            glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
        }
    }

    // ... implement other methods ...
};

// ================================================================================
// STEP 2: Ensure Your ITexture Interface Matches
// ================================================================================

/**
 * Your existing ITexture interface should have these methods.
 */
class ITexture {
public:
    virtual ~ITexture() = default;

    // Required by Material system
    virtual void bind(unsigned int textureUnit) = 0;
    virtual void unbind() = 0;

    // Your additional methods (optional)
    virtual unsigned int getWidth() const = 0;
    virtual unsigned int getHeight() const = 0;
};

/**
 * Example OpenGL implementation
 */
class OpenGLTexture : public ITexture {
    unsigned int m_textureID;
    unsigned int m_currentUnit;

public:
    void bind(unsigned int textureUnit) override {
        m_currentUnit = textureUnit;
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
    }

    void unbind() override {
        glActiveTexture(GL_TEXTURE0 + m_currentUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    unsigned int getWidth() const override { return m_width; }
    unsigned int getHeight() const override { return m_height; }

private:
    unsigned int m_width, m_height;
};

// ================================================================================
// STEP 3: Integrate with ShaderManager
// ================================================================================

/**
 * Your ShaderManager should provide non-owning pointers to Materials
 */
class ShaderManager {
public:
    /**
     * Load and compile a shader program
     * @return Non-owning pointer (ShaderManager retains ownership)
     */
    IShaderProgram* loadShader(const std::string& name,
                               const std::string& vertexPath,
                               const std::string& fragmentPath) {
        // Load and compile shader
        auto shader = std::make_unique<OpenGLShaderProgram>();
        // ... load vertex shader from vertexPath ...
        // ... load fragment shader from fragmentPath ...
        // ... link program ...

        IShaderProgram* ptr = shader.get();
        m_shaders[name] = std::move(shader);
        return ptr;
    }

    /**
     * Get shader by name
     * @return Non-owning pointer, or nullptr if not found
     */
    IShaderProgram* getShader(const std::string& name) {
        auto it = m_shaders.find(name);
        return (it != m_shaders.end()) ? it->second.get() : nullptr;
    }

    /**
     * Check if shader exists
     */
    bool hasShader(const std::string& name) const {
        return m_shaders.find(name) != m_shaders.end();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IShaderProgram>> m_shaders;
};

// ================================================================================
// STEP 4: Integrate with TextureManager
// ================================================================================

/**
 * Your TextureManager should provide shared_ptr to Materials
 */
class TextureManager {
public:
    /**
     * Load texture and return shared pointer
     * @return Shared pointer (can be used by multiple materials)
     */
    std::shared_ptr<ITexture> loadTexture(const std::string& path) {
        // Check if already loaded (cache)
        auto it = m_textures.find(path);
        if (it != m_textures.end()) {
            return it->second;  // Return cached texture
        }

        // Load new texture
        auto texture = std::make_shared<OpenGLTexture>();
        // ... load image from path ...
        // ... upload to GPU ...

        m_textures[path] = texture;
        return texture;
    }

    /**
     * Get texture by path (if already loaded)
     */
    std::shared_ptr<ITexture> getTexture(const std::string& path) const {
        auto it = m_textures.find(path);
        return (it != m_textures.end()) ? it->second : nullptr;
    }

    /**
     * Unload texture (removes from cache)
     * Texture is destroyed when all materials release their references
     */
    void unloadTexture(const std::string& path) {
        m_textures.erase(path);
    }

    /**
     * Clear all textures
     */
    void clear() {
        m_textures.clear();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<ITexture>> m_textures;
};

// ================================================================================
// STEP 5: Integrate with GameObject/Entity
// ================================================================================

/**
 * Your game objects should own their materials
 */
class GameObject {
public:
    GameObject(const std::string& name) : m_name(name) {}

    /**
     * Set material (transfers ownership)
     */
    void setMaterial(std::unique_ptr<Material> material) {
        m_material = std::move(material);
    }

    /**
     * Get material (non-owning access)
     */
    Material* getMaterial() const {
        return m_material.get();
    }

    /**
     * Check if object has valid material
     */
    bool hasMaterial() const {
        return m_material && m_material->isValid();
    }

    // ... other members (transform, mesh, etc.) ...

private:
    std::string m_name;
    std::unique_ptr<Material> m_material;  // Exclusive ownership
    // RenderMesh* m_mesh;
    // Transform m_transform;
};

// ================================================================================
// STEP 6: Integrate with Renderer
// ================================================================================

/**
 * Your renderer should use materials during rendering
 */
class Renderer {
public:
    /**
     * Render a single object with its material
     */
    void renderObject(GameObject* object, const glm::mat4& viewProj) {
        Material* material = object->getMaterial();
        if (!material || !material->isValid()) {
            return;  // Skip objects without valid materials
        }

        // Bind material (activates shader, textures, and material properties)
        material->bind();

        // Set per-object uniforms (transform, etc.)
        IShaderProgram* shader = material->getShader();
        // glm::mat4 model = object->getTransform().getMatrix();
        // shader->setMat4("u_Model", model);
        // shader->setMat4("u_MVP", viewProj * model);

        // Render geometry
        // object->getMesh()->draw();

        // Unbind material
        material->unbind();
    }

    /**
     * Render multiple objects (optimized with material sorting)
     */
    void renderScene(std::vector<GameObject*>& objects, const glm::mat4& viewProj) {
        // Sort by material to minimize state changes
        std::sort(objects.begin(), objects.end(),
            [](GameObject* a, GameObject* b) {
                return a->getMaterial() < b->getMaterial();
            });

        Material* currentMaterial = nullptr;

        for (GameObject* obj : objects) {
            Material* material = obj->getMaterial();
            if (!material || !material->isValid()) {
                continue;
            }

            // Only bind if material changed
            if (material != currentMaterial) {
                if (currentMaterial) {
                    currentMaterial->unbind();
                }
                material->bind();
                currentMaterial = material;
            }

            // Set per-object uniforms
            IShaderProgram* shader = material->getShader();
            // shader->setMat4("u_Model", obj->getTransform().getMatrix());

            // Draw
            // obj->getMesh()->draw();
        }

        // Unbind last material
        if (currentMaterial) {
            currentMaterial->unbind();
        }
    }
};

// ================================================================================
// STEP 7: Complete Integration Example
// ================================================================================

/**
 * Main application showing complete integration
 */
class Application {
public:
    void initialize() {
        // Create managers
        m_shaderManager = std::make_unique<ShaderManager>();
        m_textureManager = std::make_unique<TextureManager>();
        m_renderer = std::make_unique<Renderer>();

        // Load shaders
        m_shaderManager->loadShader("phong", "shaders/phong.vert", "shaders/phong.frag");
        m_shaderManager->loadShader("pbr", "shaders/pbr.vert", "shaders/pbr.frag");

        // Create some game objects with materials
        createRedCube();
        createMetalSphere();
        createTexturedPlane();
    }

    void createRedCube() {
        // Create game object
        auto cube = std::make_unique<GameObject>("RedCube");

        // Get shader
        IShaderProgram* phongShader = m_shaderManager->getShader("phong");

        // Create Phong material
        auto material = std::make_unique<PhongMaterial>(phongShader);
        material->setDiffuseColor(glm::vec3(1.0f, 0.0f, 0.0f));  // Red
        material->setShininess(32.0f);

        // Assign material to object
        cube->setMaterial(std::move(material));

        // Store object
        m_objects.push_back(std::move(cube));
    }

    void createMetalSphere() {
        auto sphere = std::make_unique<GameObject>("MetalSphere");

        // Get shader
        IShaderProgram* pbrShader = m_shaderManager->getShader("pbr");

        // Create PBR material
        auto material = std::make_unique<PBRMaterial>(pbrShader);
        material->setAlbedo(glm::vec3(0.95f, 0.93f, 0.88f));
        material->setMetallic(0.9f);
        material->setRoughness(0.2f);

        // Load textures
        auto albedoMap = m_textureManager->loadTexture("textures/metal_albedo.png");
        auto normalMap = m_textureManager->loadTexture("textures/metal_normal.png");

        material->setAlbedoMap(albedoMap);
        material->setNormalMap(normalMap);

        sphere->setMaterial(std::move(material));
        m_objects.push_back(std::move(sphere));
    }

    void createTexturedPlane() {
        auto plane = std::make_unique<GameObject>("TexturedPlane");

        // Get shader
        IShaderProgram* phongShader = m_shaderManager->getShader("phong");

        // Create material
        auto material = std::make_unique<PhongMaterial>(phongShader);

        // Load textures
        auto diffuseMap = m_textureManager->loadTexture("textures/brick_diffuse.png");
        auto normalMap = m_textureManager->loadTexture("textures/brick_normal.png");
        auto specularMap = m_textureManager->loadTexture("textures/brick_specular.png");

        material->setDiffuseMap(diffuseMap);
        material->setNormalMap(normalMap);
        material->setSpecularMap(specularMap);
        material->setShininess(64.0f);

        plane->setMaterial(std::move(material));
        m_objects.push_back(std::move(plane));
    }

    void update(float deltaTime) {
        // Update materials (e.g., animated properties)
        for (auto& obj : m_objects) {
            if (Material* mat = obj->getMaterial()) {
                mat->setProperty("u_Time", m_time);
            }
        }
        m_time += deltaTime;
    }

    void render() {
        // Set up camera
        glm::mat4 viewProj = getViewProjectionMatrix();

        // Render all objects
        std::vector<GameObject*> objectPtrs;
        for (auto& obj : m_objects) {
            objectPtrs.push_back(obj.get());
        }

        m_renderer->renderScene(objectPtrs, viewProj);
    }

    void shutdown() {
        // Clear objects (materials auto-destroyed)
        m_objects.clear();

        // Clear textures (destroyed when last reference released)
        m_textureManager->clear();

        // Shaders destroyed when manager destroyed
    }

private:
    glm::mat4 getViewProjectionMatrix() {
        // Get from camera
        return glm::mat4(1.0f);
    }

    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<Renderer> m_renderer;
    std::vector<std::unique_ptr<GameObject>> m_objects;
    float m_time = 0.0f;
};

// ================================================================================
// STEP 8: Advanced Integration - Material Factory
// ================================================================================

/**
 * Factory for creating common material types
 */
class MaterialFactory {
public:
    MaterialFactory(ShaderManager* shaderMgr, TextureManager* textureMgr)
        : m_shaderManager(shaderMgr), m_textureManager(textureMgr) {}

    /**
     * Create a simple colored material
     */
    std::unique_ptr<PhongMaterial> createColoredMaterial(const glm::vec3& color) {
        IShaderProgram* shader = m_shaderManager->getShader("phong");
        auto material = std::make_unique<PhongMaterial>(shader);
        material->setDiffuseColor(color);
        material->setShininess(32.0f);
        return material;
    }

    /**
     * Create a textured Phong material
     */
    std::unique_ptr<PhongMaterial> createPhongMaterial(
        const std::string& diffusePath,
        const std::string& normalPath = "",
        const std::string& specularPath = "") {

        IShaderProgram* shader = m_shaderManager->getShader("phong");
        auto material = std::make_unique<PhongMaterial>(shader);

        // Load and set textures
        if (!diffusePath.empty()) {
            auto diffuse = m_textureManager->loadTexture(diffusePath);
            material->setDiffuseMap(diffuse);
        }

        if (!normalPath.empty()) {
            auto normal = m_textureManager->loadTexture(normalPath);
            material->setNormalMap(normal);
        }

        if (!specularPath.empty()) {
            auto specular = m_textureManager->loadTexture(specularPath);
            material->setSpecularMap(specular);
        }

        return material;
    }

    /**
     * Create a PBR material
     */
    std::unique_ptr<PBRMaterial> createPBRMaterial(
        const std::string& albedoPath,
        const std::string& normalPath,
        const std::string& metallicRoughnessPath) {

        IShaderProgram* shader = m_shaderManager->getShader("pbr");
        auto material = std::make_unique<PBRMaterial>(shader);

        auto albedo = m_textureManager->loadTexture(albedoPath);
        auto normal = m_textureManager->loadTexture(normalPath);
        auto mr = m_textureManager->loadTexture(metallicRoughnessPath);

        material->setAlbedoMap(albedo);
        material->setNormalMap(normal);
        material->setMetallicRoughnessMap(mr);

        return material;
    }

private:
    ShaderManager* m_shaderManager;
    TextureManager* m_textureManager;
};

// ================================================================================
// STEP 9: Migration Guide - Converting Old Code
// ================================================================================

void MigrationExample() {
    // OLD WAY (manual shader/texture management):
    // ───────────────────────────────────────────────────────────────────────
    /*
    IShaderProgram* shader = getShader("phong");
    ITexture* diffuse = loadTexture("brick.png");
    ITexture* normal = loadTexture("brick_n.png");

    // During rendering:
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    diffuse->bind(0);
    shader->setInt("u_DiffuseMap", 0);
    glActiveTexture(GL_TEXTURE1);
    normal->bind(1);
    shader->setInt("u_NormalMap", 1);
    shader->setVec3("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));
    shader->setFloat("u_Shininess", 32.0f);
    // ... draw ...
    normal->unbind();
    diffuse->unbind();
    shader->unbind();
    */

    // NEW WAY (Material system):
    // ───────────────────────────────────────────────────────────────────────
    ShaderManager shaderMgr;
    TextureManager textureMgr;

    IShaderProgram* shader = shaderMgr.getShader("phong");
    auto diffuse = textureMgr.loadTexture("brick.png");
    auto normal = textureMgr.loadTexture("brick_n.png");

    auto material = std::make_unique<PhongMaterial>(shader);
    material->setDiffuseMap(diffuse);
    material->setNormalMap(normal);
    material->setDiffuseColor(glm::vec3(1.0f, 0.0f, 0.0f));
    material->setShininess(32.0f);

    // During rendering:
    material->bind();
    // ... draw ...
    material->unbind();

    // Benefits:
    // ✓ 90% less code at render site
    // ✓ Encapsulation (no GL calls)
    // ✓ Reusable (setup once, use many times)
    // ✓ Type-safe (compile-time checking)
}

// ================================================================================
// STEP 10: Testing Your Integration
// ================================================================================

void TestIntegration() {
    // 1. Test shader creation
    ShaderManager shaderMgr;
    IShaderProgram* shader = shaderMgr.loadShader("test", "test.vert", "test.frag");
    assert(shader != nullptr);

    // 2. Test material creation
    auto material = std::make_unique<Material>(shader);
    assert(material->isValid());

    // 3. Test texture assignment
    TextureManager textureMgr;
    auto texture = textureMgr.loadTexture("test.png");
    material->setTexture("u_DiffuseMap", texture, 0);
    assert(material->getTexture("u_DiffuseMap") == texture);

    // 4. Test property assignment
    material->setProperty("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3* color = material->getProperty<glm::vec3>("u_Color");
    assert(color != nullptr);

    // 5. Test bind/unbind
    material->bind();
    material->unbind();

    std::cout << "Integration test passed!" << std::endl;
}

// ================================================================================
// MAIN ENTRY POINT
// ================================================================================

int main() {
    // Initialize application
    Application app;
    app.initialize();

    // Game loop
    while (true /* running */) {
        float deltaTime = 0.016f;  // ~60 FPS

        app.update(deltaTime);
        app.render();

        // break;  // For example purposes
    }

    // Cleanup
    app.shutdown();

    return 0;
}
