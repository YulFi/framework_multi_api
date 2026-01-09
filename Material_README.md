# Material System Documentation

## Overview

The Material system provides a high-level abstraction for combining shaders, textures, and rendering properties into cohesive units that define surface appearance in your graphics engine.

## Architecture

### Class Hierarchy

```
Material (base class)
├── PhongMaterial (Phong lighting)
├── PBRMaterial (Physically-Based Rendering)
└── ToonMaterial (custom example)
    └── ... (your custom materials)
```

### Core Components

1. **Material** - Base class handling shader, textures, and properties
2. **TextureBinding** - Struct managing texture-sampler-unit relationships
3. **UniformValue** - Type-safe variant for shader uniform storage
4. **MaterialBuilder** - Optional builder pattern for fluent construction

## Design Decisions

### 1. Shader Ownership: Non-Owning Pointer

```cpp
IShaderProgram* m_shader;  // Non-owning
```

**Rationale:**
- Shaders are typically managed by a `ShaderManager` with their own lifecycle
- Multiple materials often share the same shader program
- Avoids premature deletion when materials are destroyed
- Clear ownership semantics: ShaderManager owns, Material observes

**Trade-off:** Materials become invalid if shader is deleted. Consider using `std::weak_ptr` if you need lifetime tracking.

### 2. Texture Ownership: Shared Pointer

```cpp
std::shared_ptr<ITexture> texture;  // Shared ownership
```

**Rationale:**
- Textures are often shared between multiple materials
- Automatic memory management via reference counting
- Safe to delete materials without worrying about texture lifetime
- Clear that textures can be reused

**Alternative:** `std::unique_ptr` if textures are never shared (less flexible).

### 3. Type-Safe Uniform Storage: std::variant

```cpp
using UniformValue = std::variant<int, float, bool, vec2, vec3, vec4, mat3, mat4>;
```

**Rationale:**
- **Type Safety:** Compile-time type checking prevents runtime errors
- **Performance:** No heap allocation, stack-based storage
- **Modern C++:** Leverages C++17 features (std::variant, std::visit)
- **Extensibility:** Easy to add new uniform types

**Trade-off:** Slightly more complex than `void*` or type erasure, but much safer.

**Alternative Considered:** `std::any` - rejected due to runtime overhead and less type safety.

### 4. Texture Unit Management: Explicit + Automatic

```cpp
// Explicit control
material->setTexture("u_DiffuseMap", texture, 0);  // Unit 0

// Automatic assignment
material->setTexture("u_DiffuseMap", texture);     // Auto-assigns next available
```

**Rationale:**
- **Flexibility:** Users can control units for performance-critical code
- **Convenience:** Auto-assignment for typical use cases
- **Validation:** Prevents unit conflicts at assignment time, not render time

**Trade-off:** Slightly more complex API, but prevents subtle bugs.

### 5. RAII Binding Pattern

```cpp
material->bind();
// Textures and shader bound here
material->unbind();
// Cleanup happens here
```

**Rationale:**
- **RAII Philosophy:** Acquisition in `bind()`, release in `unbind()`
- **Exception Safety:** Unbind in catch blocks ensures cleanup
- **Clear Lifetime:** Explicit bind/unbind scope

**Enhancement:** Consider RAII guard class:
```cpp
class MaterialBinder {
    Material* m_material;
public:
    MaterialBinder(Material* mat) : m_material(mat) { mat->bind(); }
    ~MaterialBinder() { m_material->unbind(); }
};

// Usage:
MaterialBinder binder(material.get());
// Automatic unbind on scope exit
```

### 6. Virtual Methods for Extensibility

```cpp
virtual void bind();
virtual void uploadUniform(...);
virtual void onPreBind();
virtual void onPostBind();
```

**Rationale:**
- **Extensibility:** Derived materials can customize behavior
- **Hooks:** Pre/post bind hooks for custom logic without overriding entire bind()
- **Polymorphism:** Enables material collections with varied behaviors

**Trade-off:** Virtual call overhead (negligible compared to GPU operations).

### 7. Cache-Friendly Texture Lookup

```cpp
std::vector<TextureBinding> m_textureBindings;  // Sequential access
std::unordered_map<std::string, size_t> m_samplerNameToIndex;  // O(1) lookup
```

**Rationale:**
- **Render Loop:** Sequential iteration in `bind()` is cache-friendly
- **Setup:** O(1) lookup for `getTexture()` and `setTexture()`
- **Memory Efficiency:** Small overhead for typical material (< 10 textures)

**Trade-off:** Slight complexity managing dual data structures, but significant performance benefit.

## Usage Patterns

### Pattern 1: Basic Material (Maximum Control)

```cpp
auto material = std::make_unique<Material>(shader);
material->setTexture("u_DiffuseMap", diffuseTex, 0);
material->setProperty("u_Color", glm::vec3(1.0f));
material->bind();
```

**When to Use:** Full control over every aspect, custom materials.

### Pattern 2: Specialized Materials (Convenience)

```cpp
auto material = std::make_unique<PhongMaterial>(shader);
material->setDiffuseColor(glm::vec3(1.0f, 0.0f, 0.0f));
material->setDiffuseMap(texture);
material->bind();
```

**When to Use:** Standard material types (Phong, PBR), cleaner API.

### Pattern 3: Builder Pattern (Fluent API)

```cpp
auto material = MaterialBuilder(shader)
    .withTexture("u_DiffuseMap", tex, 0)
    .withProperty("u_Color", glm::vec3(1.0f))
    .build();
```

**When to Use:** Configuration-heavy materials, declarative style.

## Best Practices

### 1. Material Sharing

```cpp
// GOOD: Share materials for identical objects
std::shared_ptr<Material> sharedMaterial = ...;
for (auto* mesh : identicalObjects) {
    sharedMaterial->bind();
    mesh->draw();
    sharedMaterial->unbind();
}

// AVOID: Creating duplicate materials
for (auto* mesh : identicalObjects) {
    auto material = std::make_unique<Material>(shader);  // Wasteful!
    // ... setup ...
}
```

### 2. Texture Reuse

```cpp
// GOOD: Share textures between materials
std::shared_ptr<ITexture> brickDiffuse = textureMgr->load("brick.png");
material1->setTexture("u_DiffuseMap", brickDiffuse, 0);
material2->setTexture("u_DiffuseMap", brickDiffuse, 0);  // Reuses texture
```

### 3. Property Updates

```cpp
// GOOD: Update properties outside bind/unbind
material->setProperty("u_Time", currentTime);
material->bind();  // Uploads updated properties
mesh->draw();
material->unbind();

// AVOID: Updating during bind
material->bind();
material->setProperty("u_Time", currentTime);  // Won't be uploaded until next bind!
mesh->draw();
material->unbind();
```

### 4. Error Handling

```cpp
// GOOD: Validate before rendering
if (!material->isValid()) {
    return;  // Skip rendering or use fallback
}

material->bind();
mesh->draw();
material->unbind();
```

### 5. Texture Unit Limits

```cpp
// Most GPUs support 16-32 texture units
// Check GL_MAX_TEXTURE_IMAGE_UNITS for actual limit

// GOOD: Use sequential units starting from 0
material->setTexture("u_Diffuse", tex1, 0);
material->setTexture("u_Normal", tex2, 1);
material->setTexture("u_Specular", tex3, 2);

// AVOID: Sparse unit allocation
material->setTexture("u_Diffuse", tex1, 0);
material->setTexture("u_Normal", tex2, 10);   // Wastes units 1-9
material->setTexture("u_Specular", tex3, 20); // Wastes units 11-19
```

## Performance Considerations

### Bind Call Overhead

```cpp
// EXPENSIVE: Bind calls involve GPU state changes
material->bind();    // Binds shader + N textures
mesh->draw();        // Draw call
material->unbind();  // Unbinds shader + N textures
```

**Optimization:** Minimize material switches by sorting draw calls:

```cpp
// Sort objects by material to reduce bind() calls
std::sort(objects.begin(), objects.end(),
    [](const auto& a, const auto& b) {
        return a.material < b.material;
    });

Material* currentMaterial = nullptr;
for (const auto& obj : objects) {
    if (obj.material != currentMaterial) {
        if (currentMaterial) currentMaterial->unbind();
        obj.material->bind();
        currentMaterial = obj.material;
    }
    obj.mesh->draw();
}
if (currentMaterial) currentMaterial->unbind();
```

### Property Storage

- `std::variant` is stack-allocated: **No heap overhead**
- `std::unordered_map` has heap allocation per entry
- **Recommendation:** For frequently updated properties, consider direct shader uniform calls instead of caching

### Texture Binding

```cpp
// Each bind() call activates all textures
material->bind();  // Calls ITexture::bind(unit) for each texture
```

**Note:** Modern OpenGL/Vulkan have bindless textures for advanced use cases.

## Extension Examples

### Custom Material: Water Material

```cpp
class WaterMaterial : public Material {
public:
    explicit WaterMaterial(IShaderProgram* shader) : Material(shader) {
        setWaveSpeed(1.0f);
        setWaveAmplitude(0.1f);
        setProperty("u_Time", 0.0f);
    }

    void setWaveSpeed(float speed) { setProperty("u_WaveSpeed", speed); }
    void setWaveAmplitude(float amp) { setProperty("u_WaveAmplitude", amp); }

    void update(float time) {
        setProperty("u_Time", time);
    }

protected:
    void onPreBind() override {
        // Enable blending for water transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void onPostBind() override {
        // Restore default state
        glDisable(GL_BLEND);
    }
};
```

### Custom Material: Decal Material

```cpp
class DecalMaterial : public Material {
public:
    explicit DecalMaterial(IShaderProgram* shader) : Material(shader) {}

protected:
    void onPreBind() override {
        // Disable depth writes for decals
        glDepthMask(GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
    }

    void onPostBind() override {
        glDepthMask(GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
};
```

## Thread Safety

**Current Design:** Not thread-safe.

**Rationale:**
- Materials are typically bound/unbound on the render thread only
- Thread-safety adds overhead (mutexes) for uncommon use case
- Clear single-threaded usage pattern

**If You Need Thread Safety:**

```cpp
class ThreadSafeMaterial : public Material {
private:
    mutable std::mutex m_mutex;

public:
    void bind() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        Material::bind();
    }

    void setProperty(const std::string& name, const UniformValue& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        Material::setProperty(name, value);
    }
    // ... lock other methods ...
};
```

## Future Enhancements

### 1. Material Serialization

```cpp
class Material {
public:
    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);
};
```

### 2. Material Instancing

```cpp
class MaterialInstance : public Material {
    std::shared_ptr<Material> m_baseMaterial;
    // Override specific properties while sharing base material
};
```

### 3. Material Hot-Reload

```cpp
class Material {
public:
    void reload() {
        // Reload shader and textures for live editing
    }
};
```

### 4. Uniform Buffers (UBO)

```cpp
class Material {
private:
    std::unique_ptr<UniformBuffer> m_ubo;  // GPU-side storage for properties
};
```

### 5. Material Graph (Node-Based)

```cpp
class MaterialNode;
class MaterialGraph {
    std::vector<MaterialNode*> nodes;
    // Shader generation from graph
};
```

## Testing Considerations

### Unit Tests

```cpp
TEST(Material, SetTextureWithExplicitUnit) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_Diffuse", texture, 0);

    EXPECT_EQ(material.getTexture("u_Diffuse"), texture);
}

TEST(Material, TextureUnitConflict) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setTexture("u_Tex1", nullptr, 0);
    EXPECT_THROW(material.setTexture("u_Tex2", nullptr, 0), std::invalid_argument);
}
```

### Integration Tests

```cpp
TEST(Material, BindCallsShaderAndTextures) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_Diffuse", texture, 0);
    material.bind();

    EXPECT_TRUE(shader.wasBound());
    EXPECT_TRUE(texture->wasBoundToUnit(0));
}
```

## Common Pitfalls

### Pitfall 1: Forgetting to Unbind

```cpp
// BAD
material1->bind();
mesh1->draw();
material2->bind();  // material1 never unbound!
mesh2->draw();

// GOOD
material1->bind();
mesh1->draw();
material1->unbind();

material2->bind();
mesh2->draw();
material2->unbind();
```

### Pitfall 2: Shader Deletion

```cpp
// BAD
{
    auto shader = std::make_unique<Shader>(...);
    material->setShader(shader.get());
}  // shader deleted here

material->bind();  // CRASH: shader pointer is dangling
```

**Solution:** Ensure shader outlives material or use weak_ptr.

### Pitfall 3: Property Type Mismatch

```cpp
material->setProperty("u_Color", glm::vec3(1.0f));

// Later...
auto* value = material->getProperty<float>("u_Color");  // Returns nullptr!
// Correct:
auto* value = material->getProperty<glm::vec3>("u_Color");  // Returns valid pointer
```

### Pitfall 4: Texture Unit Exhaustion

```cpp
// Setting 33 textures on a GPU with 32 max units
for (int i = 0; i < 33; ++i) {
    material->setTexture("u_Tex" + std::to_string(i), texture, i);  // Throws at i=32
}
```

**Solution:** Use automatic unit assignment or check limits.

## Integration with Your Engine

### With ShaderManager

```cpp
class ShaderManager {
    std::unordered_map<std::string, std::unique_ptr<IShaderProgram>> m_shaders;

public:
    IShaderProgram* getShader(const std::string& name) {
        return m_shaders[name].get();  // Non-owning pointer for Material
    }
};
```

### With TextureManager

```cpp
class TextureManager {
    std::unordered_map<std::string, std::shared_ptr<ITexture>> m_textures;

public:
    std::shared_ptr<ITexture> loadTexture(const std::string& path) {
        if (m_textures.count(path)) {
            return m_textures[path];  // Share existing texture
        }
        auto texture = createTexture(path);
        m_textures[path] = texture;
        return texture;
    }
};
```

### With RenderMesh

```cpp
class RenderMesh {
public:
    void draw(Material* material) {
        material->bind();
        // Draw geometry
        glDrawElements(...);
        material->unbind();
    }
};
```

## Conclusion

The Material system provides a robust, type-safe, and extensible foundation for managing rendering state in your graphics engine. It follows modern C++ best practices including RAII, smart pointers, and value semantics while remaining flexible enough for various rendering pipelines and material types.

**Key Strengths:**
- Type-safe uniform management
- Clear ownership semantics
- Extensible through inheritance
- Plugin-agnostic (OpenGL/Vulkan/etc.)
- Well-documented and tested

**Next Steps:**
1. Integrate with your existing shader and texture systems
2. Create specialized materials for your rendering needs
3. Implement material serialization for asset pipeline
4. Add performance profiling to optimize bind call patterns

For questions or contributions, refer to the usage examples in `MaterialUsageExample.cpp`.
