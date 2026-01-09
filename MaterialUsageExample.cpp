/**
 * @file MaterialUsageExample.cpp
 * @brief Comprehensive examples demonstrating Material class usage
 *
 * This file shows various usage patterns for the Material system including:
 * - Basic material setup
 * - PhongMaterial and PBRMaterial usage
 * - MaterialBuilder pattern
 * - Integration with rendering pipeline
 * - Custom material types
 */

#include "Material.h"
#include "IShaderProgram.h"
#include "ITexture.h"
#include <memory>
#include <glm/glm.hpp>

// Forward declarations of your existing classes
class TextureManager;
class ShaderManager;
class RenderMesh;

// ================================================================================
// Example 1: Basic Material Usage
// ================================================================================

void Example_BasicMaterial(IShaderProgram* shader,
                          std::shared_ptr<ITexture> diffuseTexture,
                          std::shared_ptr<ITexture> normalTexture) {
    // Create material with shader
    auto material = std::make_unique<Material>(shader);

    // Set textures with explicit units
    material->setTexture("u_DiffuseMap", diffuseTexture, 0);
    material->setTexture("u_NormalMap", normalTexture, 1);

    // Set material properties
    material->setProperty("u_Color", glm::vec3(1.0f, 0.8f, 0.6f));
    material->setProperty("u_Shininess", 32.0f);
    material->setProperty("u_HasNormalMap", true);

    // Rendering
    material->bind();
    // ... draw your geometry ...
    material->unbind();
}

// ================================================================================
// Example 2: PhongMaterial for Standard Lighting
// ================================================================================

void Example_PhongMaterial(IShaderProgram* phongShader,
                          TextureManager* textureMgr) {
    // Create Phong material with convenient API
    auto material = std::make_unique<PhongMaterial>(phongShader);

    // Set material properties using typed setters
    material->setDiffuseColor(glm::vec3(0.8f, 0.2f, 0.2f));  // Red-ish
    material->setSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f)); // White specular
    material->setShininess(64.0f);                            // Shiny surface

    // Load and set textures (assuming TextureManager returns shared_ptr)
    auto diffuseTex = textureMgr->loadTexture("assets/textures/brick_diffuse.png");
    auto normalTex = textureMgr->loadTexture("assets/textures/brick_normal.png");
    auto specularTex = textureMgr->loadTexture("assets/textures/brick_specular.png");

    material->setDiffuseMap(diffuseTex);
    material->setNormalMap(normalTex);
    material->setSpecularMap(specularTex);

    // PhongMaterial automatically manages texture units and flags
    // Now u_HasDiffuseMap, u_HasNormalMap, u_HasSpecularMap are all set to true

    // Use in render loop
    material->bind();
    // ... render mesh ...
    material->unbind();
}

// ================================================================================
// Example 3: PBR Material for Physically-Based Rendering
// ================================================================================

void Example_PBRMaterial(IShaderProgram* pbrShader,
                        TextureManager* textureMgr) {
    // Create PBR material
    auto material = std::make_unique<PBRMaterial>(pbrShader);

    // Set base PBR properties
    material->setAlbedo(glm::vec3(0.95f, 0.93f, 0.88f)); // Off-white base color
    material->setMetallic(0.8f);                          // Mostly metallic
    material->setRoughness(0.3f);                         // Fairly smooth
    material->setAO(1.0f);                                // Full ambient occlusion

    // Load PBR texture maps
    auto albedoMap = textureMgr->loadTexture("assets/pbr/metal_albedo.png");
    auto normalMap = textureMgr->loadTexture("assets/pbr/metal_normal.png");
    auto aoMap = textureMgr->loadTexture("assets/pbr/metal_ao.png");

    // Option 1: Separate metallic and roughness maps
    auto metallicMap = textureMgr->loadTexture("assets/pbr/metal_metallic.png");
    auto roughnessMap = textureMgr->loadTexture("assets/pbr/metal_roughness.png");

    material->setAlbedoMap(albedoMap);
    material->setNormalMap(normalMap);
    material->setMetallicMap(metallicMap);
    material->setRoughnessMap(roughnessMap);
    material->setAOMap(aoMap);

    // Option 2: Combined metallic-roughness map (GLTF standard)
    // auto metallicRoughnessMap = textureMgr->loadTexture("assets/pbr/metal_metallic_roughness.png");
    // material->setMetallicRoughnessMap(metallicRoughnessMap);
    // This automatically disables separate metallic/roughness maps

    material->bind();
    // ... render PBR mesh ...
    material->unbind();
}

// ================================================================================
// Example 4: MaterialBuilder for Fluent Construction
// ================================================================================

std::unique_ptr<Material> Example_MaterialBuilder(IShaderProgram* shader,
                                                  std::shared_ptr<ITexture> tex1,
                                                  std::shared_ptr<ITexture> tex2) {
    // Fluent API for cleaner material setup
    return MaterialBuilder(shader)
        .withTexture("u_DiffuseMap", tex1, 0)
        .withTexture("u_NormalMap", tex2, 1)
        .withProperty("u_Color", glm::vec3(1.0f, 1.0f, 1.0f))
        .withProperty("u_Shininess", 32.0f)
        .withProperty("u_Metallic", 0.5f)
        .build();
}

// ================================================================================
// Example 5: Custom Material Type
// ================================================================================

/**
 * @brief Custom toon/cel-shaded material
 */
class ToonMaterial : public Material {
public:
    explicit ToonMaterial(IShaderProgram* shader)
        : Material(shader) {
        // Initialize toon-specific defaults
        setOutlineColor(glm::vec3(0.0f, 0.0f, 0.0f));
        setOutlineThickness(0.02f);
        setShadingLevels(3); // Discrete shading levels
    }

    void setOutlineColor(const glm::vec3& color) {
        setProperty("u_OutlineColor", color);
    }

    void setOutlineThickness(float thickness) {
        setProperty("u_OutlineThickness", thickness);
    }

    void setShadingLevels(int levels) {
        setProperty("u_ShadingLevels", levels);
    }

    void setToonRamp(std::shared_ptr<ITexture> rampTexture) {
        if (rampTexture) {
            setTexture("u_ToonRamp", rampTexture, 0);
            setProperty("u_UseToonRamp", true);
        } else {
            removeTexture("u_ToonRamp");
            setProperty("u_UseToonRamp", false);
        }
    }

protected:
    // Override for custom toon shading logic
    void onPreBind() override {
        // Custom pre-bind logic if needed
        // e.g., disable certain OpenGL states, set custom uniforms, etc.
    }
};

void Example_CustomMaterial(IShaderProgram* toonShader,
                           std::shared_ptr<ITexture> rampTex) {
    auto material = std::make_unique<ToonMaterial>(toonShader);

    material->setOutlineColor(glm::vec3(0.0f, 0.0f, 0.0f));
    material->setOutlineThickness(0.03f);
    material->setShadingLevels(4);
    material->setToonRamp(rampTex);

    material->bind();
    // ... render with toon shading ...
    material->unbind();
}

// ================================================================================
// Example 6: Integration with Render Pipeline
// ================================================================================

class Renderer {
public:
    void renderMesh(RenderMesh* mesh, Material* material, const glm::mat4& transform) {
        if (!material || !material->isValid()) {
            // Fallback to default material or skip
            return;
        }

        // Bind material (activates shader, textures, and properties)
        material->bind();

        // Set per-object uniforms (transform, etc.)
        IShaderProgram* shader = material->getShader();
        shader->setMat4("u_Model", transform);
        // Additional per-object uniforms...

        // Render the mesh
        mesh->draw();

        // Unbind material
        material->unbind();
    }

    void renderScene(const std::vector<std::pair<RenderMesh*, Material*>>& objects) {
        for (const auto& [mesh, material] : objects) {
            glm::mat4 transform = glm::mat4(1.0f); // Get from scene graph
            renderMesh(mesh, material, transform);
        }
    }
};

// ================================================================================
// Example 7: Dynamic Material Modification
// ================================================================================

void Example_DynamicMaterialUpdate(Material* material, float deltaTime) {
    // Animate material properties over time
    static float time = 0.0f;
    time += deltaTime;

    // Pulsing emissive color
    float intensity = 0.5f + 0.5f * std::sin(time * 2.0f);
    material->setProperty("u_EmissiveColor", glm::vec3(1.0f, 0.5f, 0.0f) * intensity);

    // Rotating normal map (via texture matrix or offset)
    glm::vec2 uvOffset(std::cos(time * 0.5f), std::sin(time * 0.5f));
    material->setProperty("u_UVOffset", uvOffset * 0.1f);
}

// ================================================================================
// Example 8: Material Sharing and Instancing
// ================================================================================

void Example_MaterialSharing(ShaderManager* shaderMgr, TextureManager* textureMgr) {
    // Create a single material shared by multiple objects
    auto shader = shaderMgr->getShader("phong");
    auto sharedMaterial = std::make_unique<PhongMaterial>(shader);

    sharedMaterial->setDiffuseColor(glm::vec3(0.7f, 0.7f, 0.7f));
    sharedMaterial->setShininess(32.0f);

    // Multiple objects can use the same material (read-only usage)
    std::vector<RenderMesh*> meshes = { /* ... */ };

    for (auto* mesh : meshes) {
        sharedMaterial->bind();
        // Set per-object transforms
        // mesh->draw();
        sharedMaterial->unbind();
    }

    // Note: For write operations, each object should have its own material instance
}

// ================================================================================
// Example 9: Texture Swapping
// ================================================================================

void Example_TextureSwapping(Material* material,
                            std::shared_ptr<ITexture> summerTex,
                            std::shared_ptr<ITexture> winterTex,
                            bool isSummer) {
    // Dynamically swap textures based on game state
    if (isSummer) {
        material->setTexture("u_DiffuseMap", summerTex, 0);
    } else {
        material->setTexture("u_DiffuseMap", winterTex, 0);
    }

    // The texture unit (0) remains the same, only the texture changes
}

// ================================================================================
// Example 10: Material Property Querying
// ================================================================================

void Example_MaterialQuerying(const Material* material) {
    // Query material properties for editor or debugging
    if (const auto* color = material->getProperty<glm::vec3>("u_Color")) {
        // Color is available, do something with it
        std::cout << "Material color: " << color->r << ", " << color->g << ", " << color->b << std::endl;
    }

    if (const auto* shininess = material->getProperty<float>("u_Shininess")) {
        std::cout << "Shininess: " << *shininess << std::endl;
    }

    // Check texture availability
    auto diffuseTex = material->getTexture("u_DiffuseMap");
    if (diffuseTex) {
        std::cout << "Diffuse texture is set" << std::endl;
    }

    // Iterate all texture bindings
    for (const auto& binding : material->getTextureBindings()) {
        std::cout << "Sampler: " << binding.samplerName
                  << ", Unit: " << binding.textureUnit << std::endl;
    }
}

// ================================================================================
// Example 11: Error Handling
// ================================================================================

void Example_ErrorHandling(IShaderProgram* shader) {
    try {
        // Attempt to create material with null shader (will throw)
        auto material = std::make_unique<Material>(nullptr);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Failed to create material: " << e.what() << std::endl;
    }

    auto material = std::make_unique<Material>(shader);

    // Set texture with unit 0
    material->setTexture("u_Texture1", nullptr, 0);

    try {
        // Attempt to set another texture to the same unit (will throw)
        material->setTexture("u_Texture2", nullptr, 0);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Texture unit conflict: " << e.what() << std::endl;
    }

    // Correct approach: use automatic unit assignment
    material->setTexture("u_Texture1", nullptr); // Gets unit 0
    material->setTexture("u_Texture2", nullptr); // Gets unit 1 automatically
}

// ================================================================================
// Example 12: Complete Rendering Example
// ================================================================================

void Example_CompleteRenderingWorkflow() {
    // Assume these are initialized elsewhere
    ShaderManager* shaderMgr = nullptr;
    TextureManager* textureMgr = nullptr;
    RenderMesh* cubeMesh = nullptr;

    // 1. Get shader from manager
    IShaderProgram* pbrShader = shaderMgr->getShader("pbr_shader");

    // 2. Create material
    auto material = std::make_unique<PBRMaterial>(pbrShader);

    // 3. Configure material
    material->setAlbedo(glm::vec3(0.8f, 0.1f, 0.1f));
    material->setMetallic(0.9f);
    material->setRoughness(0.2f);

    // 4. Load and set textures
    material->setAlbedoMap(textureMgr->loadTexture("metal_albedo.png"));
    material->setNormalMap(textureMgr->loadTexture("metal_normal.png"));
    material->setMetallicRoughnessMap(textureMgr->loadTexture("metal_mr.png"));

    // 5. Render loop
    while (true /* rendering */) {
        // Update per-frame material properties if needed
        // material->setProperty("u_Time", currentTime);

        // Bind material
        material->bind();

        // Set per-object uniforms (MVP matrices, etc.)
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);  // Get from camera
        glm::mat4 proj = glm::mat4(1.0f);  // Get from camera

        pbrShader->setMat4("u_Model", model);
        pbrShader->setMat4("u_View", view);
        pbrShader->setMat4("u_Projection", proj);

        // Draw mesh
        cubeMesh->draw();

        // Unbind material
        material->unbind();

        // break; // Exit for example purposes
    }
}

// ================================================================================
// Main Example Dispatcher
// ================================================================================

int main() {
    // Initialize your rendering system
    // ShaderManager shaderMgr;
    // TextureManager textureMgr;
    // ...

    // Run examples
    // Example_BasicMaterial(...);
    // Example_PhongMaterial(...);
    // Example_PBRMaterial(...);
    // etc.

    return 0;
}
