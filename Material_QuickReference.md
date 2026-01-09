# Material System Quick Reference

## Quick Start (30 Seconds)

```cpp
// 1. Create material
auto material = std::make_unique<Material>(shaderProgram);

// 2. Add textures
material->setTexture("u_DiffuseMap", diffuseTexture, 0);

// 3. Set properties
material->setProperty("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));

// 4. Use in render loop
material->bind();
mesh->draw();
material->unbind();
```

## Common Operations

### Creating Materials

```cpp
// Basic material
auto material = std::make_unique<Material>(shader);

// Phong material (for standard lighting)
auto phong = std::make_unique<PhongMaterial>(shader);

// PBR material (for physically-based rendering)
auto pbr = std::make_unique<PBRMaterial>(shader);

// With builder pattern
auto material = MaterialBuilder(shader)
    .withTexture("u_DiffuseMap", texture, 0)
    .withProperty("u_Color", glm::vec3(1.0f))
    .build();
```

### Setting Textures

```cpp
// With explicit texture unit
material->setTexture("u_DiffuseMap", texture, 0);
material->setTexture("u_NormalMap", normalTex, 1);

// With automatic unit assignment
unsigned int unit1 = material->setTexture("u_DiffuseMap", texture);
unsigned int unit2 = material->setTexture("u_NormalMap", normalTex);

// Remove texture
material->removeTexture("u_DiffuseMap");

// Get texture
auto tex = material->getTexture("u_DiffuseMap");
if (tex) {
    // Texture exists
}
```

### Setting Properties

```cpp
// Scalar properties
material->setProperty("u_Shininess", 32.0f);
material->setProperty("u_Metallic", 0.8f);
material->setProperty("u_UseTexture", true);

// Vector properties
material->setProperty("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));
material->setProperty("u_Tint", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));

// Matrix properties
material->setProperty("u_TextureMatrix", glm::mat4(1.0f));

// Remove property
material->removeProperty("u_Shininess");

// Get property (type-safe)
if (const auto* color = material->getProperty<glm::vec3>("u_Color")) {
    // Use *color
}
```

### Rendering

```cpp
// Basic usage
material->bind();
mesh->draw();
material->unbind();

// Check validity
if (material->isValid()) {
    material->bind();
    // ... render ...
    material->unbind();
}

// Multiple objects with same material
material->bind();
for (auto* mesh : meshes) {
    // Set per-object uniforms
    material->getShader()->setMat4("u_Model", mesh->getTransform());
    mesh->draw();
}
material->unbind();
```

## PhongMaterial API

```cpp
auto material = std::make_unique<PhongMaterial>(shader);

// Colors
material->setDiffuseColor(glm::vec3(0.8f, 0.2f, 0.2f));
material->setSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));
material->setShininess(64.0f);

// Textures (auto-assigns standard units)
material->setDiffuseMap(diffuseTexture);    // Unit 0
material->setSpecularMap(specularTexture);  // Unit 1
material->setNormalMap(normalTexture);      // Unit 2

// Automatically sets u_HasDiffuseMap, u_HasSpecularMap, u_HasNormalMap
```

## PBRMaterial API

```cpp
auto material = std::make_unique<PBRMaterial>(shader);

// PBR properties
material->setAlbedo(glm::vec3(0.95f, 0.93f, 0.88f));
material->setMetallic(0.8f);
material->setRoughness(0.3f);
material->setAO(1.0f);

// Textures (auto-assigns standard units)
material->setAlbedoMap(albedoTexture);      // Unit 0
material->setNormalMap(normalTexture);      // Unit 1
material->setMetallicMap(metallicTexture);  // Unit 2
material->setRoughnessMap(roughnessTexture);// Unit 3
material->setAOMap(aoTexture);              // Unit 4

// OR use combined metallic-roughness map (GLTF standard)
material->setMetallicRoughnessMap(mrTexture);  // Unit 2

// Automatically sets u_HasAlbedoMap, u_HasMetallicRoughnessMap, etc.
```

## Property Types

```cpp
// Supported types in std::variant
material->setProperty("name", 42);                          // int
material->setProperty("name", 3.14f);                       // float
material->setProperty("name", true);                        // bool
material->setProperty("name", glm::vec2(1.0f, 2.0f));      // vec2
material->setProperty("name", glm::vec3(1.0f, 2.0f, 3.0f));// vec3
material->setProperty("name", glm::vec4(1.0f, 2.0f, 3.0f, 4.0f)); // vec4
material->setProperty("name", glm::mat3(1.0f));            // mat3
material->setProperty("name", glm::mat4(1.0f));            // mat4
```

## Advanced Usage

### Custom Material Type

```cpp
class MyCustomMaterial : public Material {
public:
    explicit MyCustomMaterial(IShaderProgram* shader) : Material(shader) {
        // Initialize defaults
        setCustomProperty(42.0f);
    }

    void setCustomProperty(float value) {
        setProperty("u_CustomProp", value);
    }

protected:
    void onPreBind() override {
        // Custom logic before binding
    }

    void onPostBind() override {
        // Custom logic after binding
    }
};
```

### Material Sorting (Performance Optimization)

```cpp
// Sort objects by material to minimize bind() calls
struct RenderObject {
    RenderMesh* mesh;
    Material* material;
    glm::mat4 transform;
};

std::vector<RenderObject> objects;

// Sort
std::sort(objects.begin(), objects.end(),
    [](const RenderObject& a, const RenderObject& b) {
        return a.material < b.material;
    });

// Render with minimal material switches
Material* currentMaterial = nullptr;
for (const auto& obj : objects) {
    if (obj.material != currentMaterial) {
        if (currentMaterial) currentMaterial->unbind();
        obj.material->bind();
        currentMaterial = obj.material;
    }
    // Set per-object uniforms
    obj.material->getShader()->setMat4("u_Model", obj.transform);
    obj.mesh->draw();
}
if (currentMaterial) currentMaterial->unbind();
```

### RAII Material Binder

```cpp
// For exception-safe automatic unbinding
class MaterialBinder {
    Material* m_material;
public:
    explicit MaterialBinder(Material* mat) : m_material(mat) {
        if (m_material) m_material->bind();
    }
    ~MaterialBinder() {
        if (m_material) m_material->unbind();
    }
    MaterialBinder(const MaterialBinder&) = delete;
    MaterialBinder& operator=(const MaterialBinder&) = delete;
};

// Usage:
{
    MaterialBinder binder(material.get());
    mesh->draw();
}  // Automatic unbind on scope exit
```

### Dynamic Property Updates

```cpp
// Update material properties per frame
void updateMaterial(Material* material, float deltaTime) {
    static float time = 0.0f;
    time += deltaTime;

    // Animated properties
    float pulse = 0.5f + 0.5f * std::sin(time * 2.0f);
    material->setProperty("u_Intensity", pulse);

    glm::vec2 uvScroll(time * 0.1f, 0.0f);
    material->setProperty("u_UVOffset", uvScroll);
}
```

## Troubleshooting

### Problem: Material not rendering correctly

```cpp
// Check validity
if (!material->isValid()) {
    std::cerr << "Material has null shader!" << std::endl;
}

// Verify shader is bound
IShaderProgram* shader = material->getShader();
if (!shader) {
    std::cerr << "Shader is null!" << std::endl;
}

// Check textures
auto tex = material->getTexture("u_DiffuseMap");
if (!tex) {
    std::cerr << "Diffuse texture not set!" << std::endl;
}
```

### Problem: Texture unit conflict

```cpp
try {
    material->setTexture("u_Tex1", texture1, 0);
    material->setTexture("u_Tex2", texture2, 0);  // Throws!
} catch (const std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    // Use automatic assignment instead
    material->setTexture("u_Tex2", texture2);
}
```

### Problem: Wrong property type

```cpp
material->setProperty("u_Color", glm::vec3(1.0f));

// Wrong type query returns nullptr
const float* value = material->getProperty<float>("u_Color");  // nullptr!

// Correct type query
const glm::vec3* color = material->getProperty<glm::vec3>("u_Color");  // Valid!
if (color) {
    std::cout << "Color: " << color->r << ", " << color->g << ", " << color->b << std::endl;
}
```

### Problem: Forgot to unbind

```cpp
// BAD: Next material won't render correctly
material1->bind();
mesh1->draw();
material2->bind();  // material1 still bound!

// GOOD: Always unbind
material1->bind();
mesh1->draw();
material1->unbind();

material2->bind();
mesh2->draw();
material2->unbind();

// BETTER: Use RAII binder
{
    MaterialBinder binder(material1.get());
    mesh1->draw();
}  // Auto-unbind
```

## Performance Tips

1. **Minimize bind() calls** - Sort by material and batch draw calls
2. **Reuse materials** - Share materials between identical objects
3. **Update properties outside bind()** - Set before bind, not during
4. **Use shared textures** - Multiple materials can share same texture
5. **Preallocate materials** - Create during loading, not per frame

## Integration Checklist

- [ ] IShaderProgram has bind(), unbind(), setInt(), setFloat(), setVec3(), setMat4()
- [ ] ITexture has bind(unit), unbind()
- [ ] Include glm/glm.hpp for vector/matrix types
- [ ] ShaderManager provides IShaderProgram* (non-owning)
- [ ] TextureManager provides std::shared_ptr<ITexture>
- [ ] RenderMesh can call draw()

## Common Shader Uniforms

### Phong Shader
```glsl
uniform sampler2D u_DiffuseMap;
uniform sampler2D u_SpecularMap;
uniform sampler2D u_NormalMap;
uniform vec3 u_Diffuse;
uniform vec3 u_Specular;
uniform float u_Shininess;
uniform bool u_HasDiffuseMap;
uniform bool u_HasSpecularMap;
uniform bool u_HasNormalMap;
```

### PBR Shader
```glsl
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;
uniform bool u_HasAlbedoMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicMap;
uniform bool u_HasRoughnessMap;
uniform bool u_HasAOMap;
uniform bool u_HasMetallicRoughnessMap;
```

## File Reference

- **Material.h** - Header with class declarations
- **Material.cpp** - Implementation
- **MaterialUsageExample.cpp** - Comprehensive examples
- **Material_README.md** - Full documentation
- **Material_DesignAlternatives.md** - Design trade-offs
- **Material_QuickReference.md** - This file

## Next Steps

1. Integrate with your shader and texture systems
2. Create materials for your objects
3. Implement custom material types as needed
4. Profile and optimize material binding patterns
5. Add serialization for asset pipeline

---

**Need more help?** See MaterialUsageExample.cpp for 12+ complete examples.
