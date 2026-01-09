# Material System: Design Alternatives and Trade-offs

This document explores alternative design decisions that were considered during the Material class architecture and explains why the chosen approach was selected.

## 1. Shader Ownership

### Alternative A: Raw Pointer (Chosen)

```cpp
class Material {
    IShaderProgram* m_shader;  // Non-owning
};
```

**Pros:**
- Clear non-owning semantics
- Simple and lightweight
- Matches typical ShaderManager ownership pattern
- No reference counting overhead

**Cons:**
- No lifetime tracking - can dangle
- Requires external validation
- No automatic cleanup

### Alternative B: Shared Pointer

```cpp
class Material {
    std::shared_ptr<IShaderProgram> m_shader;  // Shared ownership
};
```

**Pros:**
- Automatic lifetime management
- Can't dangle
- Clear shared ownership

**Cons:**
- Reference counting overhead
- Unclear who truly owns shader
- Complicates ShaderManager design
- Shaders rarely destroyed individually

### Alternative C: Weak Pointer

```cpp
class Material {
    std::weak_ptr<IShaderProgram> m_shader;  // Observed, with tracking
};
```

**Pros:**
- Lifetime tracking without ownership
- Can detect when shader is deleted
- Safe against dangling

**Cons:**
- Requires ShaderManager to use shared_ptr
- Must lock() before every use (overhead)
- More complex API

**Decision:** **Raw pointer** is most appropriate because:
1. ShaderManager clearly owns shaders
2. Shaders have well-defined lifetime (program duration typically)
3. Simple and efficient
4. If lifetime tracking needed, weak_ptr can be added later

---

## 2. Texture Ownership

### Alternative A: Shared Pointer (Chosen)

```cpp
struct TextureBinding {
    std::shared_ptr<ITexture> texture;
};
```

**Pros:**
- Natural sharing between materials
- Automatic cleanup when last reference dies
- Clear shared ownership semantics
- Safe against premature deletion

**Cons:**
- Reference counting overhead (atomic operations)
- Slightly larger memory footprint

### Alternative B: Unique Pointer

```cpp
struct TextureBinding {
    std::unique_ptr<ITexture> texture;  // Exclusive ownership
};
```

**Pros:**
- No reference counting overhead
- Clear exclusive ownership
- Smaller memory footprint

**Cons:**
- Can't share textures between materials
- Must clone textures for multiple materials
- Wasteful for common textures (brick wall used 100x)

### Alternative C: Raw Pointer

```cpp
struct TextureBinding {
    ITexture* texture;  // Non-owning
};
```

**Pros:**
- Smallest memory footprint
- No ownership overhead
- Matches shader ownership pattern

**Cons:**
- Requires external TextureManager
- Can dangle if texture deleted
- Unclear lifetime management

**Decision:** **Shared pointer** because:
1. Textures are commonly shared (diffuse map used by many materials)
2. Reference counting overhead negligible compared to GPU operations
3. Clear ownership without manual management
4. Prevents accidental deletion while material still uses texture

---

## 3. Uniform Value Storage

### Alternative A: std::variant (Chosen)

```cpp
using UniformValue = std::variant<int, float, bool, vec2, vec3, vec4, mat3, mat4>;
std::unordered_map<std::string, UniformValue> m_properties;
```

**Pros:**
- Type-safe at compile and runtime
- No heap allocation (stack-based)
- Modern C++ (C++17)
- Pattern matching with std::visit
- Fixed memory size

**Cons:**
- Limited to predefined types
- Larger per-value memory (size of largest type)
- Slightly complex template metaprogramming

### Alternative B: std::any

```cpp
std::unordered_map<std::string, std::any> m_properties;
```

**Pros:**
- Can store ANY type (ultimate flexibility)
- Easy to extend with custom types

**Cons:**
- Type erasure with runtime overhead
- Heap allocation for large types
- Less type-safe (runtime cast required)
- Larger memory footprint
- Harder to serialize

### Alternative C: void* with Type Enum

```cpp
enum class UniformType { INT, FLOAT, VEC3, ... };

struct UniformValue {
    void* data;
    UniformType type;
};
```

**Pros:**
- Explicit type tracking
- Minimal memory overhead
- C-style simplicity

**Cons:**
- NOT type-safe (casts required)
- Manual memory management
- Easy to get wrong (type mismatch bugs)
- Not modern C++

### Alternative D: Template Specialization

```cpp
template<typename T>
class TypedUniform {
    T value;
};

std::unordered_map<std::string, std::unique_ptr<BaseUniform>> m_properties;
```

**Pros:**
- Type-safe
- Flexible for custom types

**Cons:**
- Heap allocation per uniform
- Complex inheritance hierarchy
- Verbose usage

**Decision:** **std::variant** because:
1. Type safety is critical for shader uniforms
2. Stack allocation is faster than heap
3. Fixed set of types (int, float, vectors, matrices) is known
4. Modern C++ idiom with good compiler support
5. Easy to extend if more types needed

---

## 4. Texture Binding Storage

### Alternative A: Vector + Map (Chosen)

```cpp
std::vector<TextureBinding> m_textureBindings;  // Iteration
std::unordered_map<std::string, size_t> m_samplerNameToIndex;  // Lookup
```

**Pros:**
- O(1) lookup by name
- Cache-friendly iteration (sequential in bind())
- Optimal for both access patterns

**Cons:**
- Dual data structures (slight complexity)
- Must maintain consistency between vector and map
- Slightly higher memory usage

### Alternative B: Map Only

```cpp
std::unordered_map<std::string, TextureBinding> m_textureBindings;
```

**Pros:**
- Single data structure (simpler)
- O(1) lookup by name
- Easy to add/remove

**Cons:**
- Non-sequential iteration (cache-unfriendly)
- Iteration order not guaranteed
- Heap allocation per entry

### Alternative C: Vector Only

```cpp
std::vector<TextureBinding> m_textureBindings;
```

**Pros:**
- Single data structure
- Cache-friendly iteration
- Minimal memory overhead

**Cons:**
- O(n) lookup by name (linear search)
- Inefficient for getTexture(), removeTexture()

**Decision:** **Vector + Map** because:
1. `bind()` is called every frame → cache-friendly iteration critical
2. `getTexture()`, `setTexture()` are called during setup → O(1) lookup helpful
3. Typical materials have < 10 textures → memory overhead is small
4. Performance gain in render loop justifies complexity

**Performance Impact:**
```cpp
// Render loop (every frame, thousands of times):
for (const auto& binding : m_textureBindings) {  // Sequential, cache-friendly
    binding.texture->bind(binding.textureUnit);
}

// Setup (once):
auto tex = getTexture("u_DiffuseMap");  // O(1) via map lookup
```

---

## 5. Bind/Unbind vs RAII Guard

### Alternative A: Explicit bind/unbind (Chosen)

```cpp
material->bind();
mesh->draw();
material->unbind();
```

**Pros:**
- Explicit control
- Familiar OpenGL-style API
- Easy to debug (clear call points)
- No hidden allocations

**Cons:**
- Easy to forget unbind()
- Exception-unsafe without try-catch
- Verbose

### Alternative B: RAII Guard

```cpp
class MaterialBinder {
    Material* m_material;
public:
    MaterialBinder(Material* mat) : m_material(mat) { mat->bind(); }
    ~MaterialBinder() { m_material->unbind(); }
    MaterialBinder(const MaterialBinder&) = delete;
};

// Usage:
{
    MaterialBinder binder(material.get());
    mesh->draw();
}  // Automatic unbind
```

**Pros:**
- Exception-safe (unbind in destructor)
- Can't forget unbind
- True RAII pattern

**Cons:**
- Less explicit
- Overhead of guard object creation
- Harder to debug (destructor called at scope exit)

### Alternative C: Callable with Lambda

```cpp
material->withBinding([&]() {
    mesh->draw();
});
```

**Pros:**
- Exception-safe
- Encapsulated scope
- Functional style

**Cons:**
- Unfamiliar API for graphics programmers
- Lambda overhead
- Harder to debug

**Decision:** **Explicit bind/unbind** with **optional RAII guard** because:
1. Matches OpenGL/Vulkan conventions (familiar to graphics programmers)
2. Clear and debuggable
3. Users can add RAII guard if desired:
   ```cpp
   class MaterialBinder { /* as above */ };
   MaterialBinder binder(material.get());
   ```
4. No forced allocation or lambda overhead

---

## 6. Property Update Strategy

### Alternative A: Lazy Update on bind() (Chosen)

```cpp
void Material::bind() {
    for (const auto& [name, value] : m_properties) {
        uploadUniform(name, value);  // Upload all properties every bind
    }
}
```

**Pros:**
- Simple implementation
- Properties always in sync with shader
- Easy to modify properties between frames
- No dirty tracking needed

**Cons:**
- Uploads unchanged properties every frame
- Potential GPU overhead for large property sets

### Alternative B: Dirty Flag Tracking

```cpp
class Material {
    std::unordered_map<std::string, UniformValue> m_properties;
    std::unordered_set<std::string> m_dirtyProperties;

public:
    void setProperty(const std::string& name, const UniformValue& value) {
        m_properties[name] = value;
        m_dirtyProperties.insert(name);
    }

    void bind() {
        for (const auto& name : m_dirtyProperties) {
            uploadUniform(name, m_properties[name]);
        }
        m_dirtyProperties.clear();
    }
};
```

**Pros:**
- Only uploads changed properties
- More efficient for large property sets
- Reduces GPU calls

**Cons:**
- Increased complexity
- More memory (dirty set)
- Potential bugs if dirty tracking fails

### Alternative C: Immediate Upload

```cpp
void Material::setProperty(const std::string& name, const UniformValue& value) {
    m_properties[name] = value;

    if (m_shader) {
        m_shader->bind();
        uploadUniform(name, value);  // Upload immediately
        m_shader->unbind();
    }
}
```

**Pros:**
- Properties always synchronized with GPU
- No deferred upload complexity

**Cons:**
- Requires shader bind/unbind for every property change
- Extremely inefficient for bulk updates
- Fragmented state changes

**Decision:** **Lazy update on bind()** because:
1. Simplicity outweighs premature optimization
2. Typical materials have < 20 properties → upload cost is negligible
3. Modern GPUs have efficient uniform update paths
4. Dirty tracking can be added later if profiling shows bottleneck
5. Clean separation: setup phase (set properties) vs render phase (bind/upload)

**Profiling Note:** If profiling reveals uniform upload as bottleneck, upgrade to dirty tracking.

---

## 7. Specialized Materials: Inheritance vs Composition

### Alternative A: Inheritance (Chosen)

```cpp
class PhongMaterial : public Material {
public:
    void setDiffuseColor(const glm::vec3& color);
    void setDiffuseMap(std::shared_ptr<ITexture> texture);
};
```

**Pros:**
- Type-safe material categories
- Convenient typed APIs
- Can override virtual methods (bind, uploadUniform)
- Polymorphic collections

**Cons:**
- Class explosion for many material types
- Inheritance hierarchy complexity
- Virtual call overhead (negligible)

### Alternative B: Composition with Material Templates

```cpp
class MaterialTemplate {
    std::vector<PropertyDescriptor> properties;
    std::vector<TextureSlot> textureSlots;
};

class Material {
    MaterialTemplate* m_template;
    // ... apply template ...
};
```

**Pros:**
- Data-driven materials
- No class per material type
- Easy to add new material types (JSON, etc.)
- Smaller code footprint

**Cons:**
- No compile-time type checking
- Runtime overhead (property lookup)
- Less discoverable API
- Harder to debug

### Alternative C: Policy-Based Design

```cpp
template<typename ColorPolicy, typename TexturePolicy>
class Material : public ColorPolicy, public TexturePolicy {
    // Policies provide different behaviors
};

using PhongMaterial = Material<PhongColorPolicy, PhongTexturePolicy>;
```

**Pros:**
- Extremely flexible
- Compile-time polymorphism (no virtual calls)
- Zero-overhead abstractions

**Cons:**
- Template complexity explosion
- Long compile times
- Difficult to understand for most developers
- Overkill for graphics materials

**Decision:** **Inheritance** because:
1. Material types are relatively stable (Phong, PBR, Toon, etc.)
2. Type-safe APIs improve usability
3. Virtual call overhead is negligible compared to GPU operations
4. Clear and familiar pattern for graphics programmers
5. Easy to document and understand

**Future:** If data-driven materials are needed (user-defined shaders), add template system alongside inheritance.

---

## 8. Texture Unit Assignment

### Alternative A: Manual + Automatic (Chosen)

```cpp
// Manual control
material->setTexture("u_DiffuseMap", texture, 0);

// Automatic assignment
unsigned int unit = material->setTexture("u_DiffuseMap", texture);
```

**Pros:**
- Flexibility for both use cases
- Explicit control when needed
- Convenience when not
- Compile-time validation of unit conflicts

**Cons:**
- Two APIs for same operation (slight complexity)

### Alternative B: Manual Only

```cpp
material->setTexture("u_DiffuseMap", texture, 0);  // Always specify unit
```

**Pros:**
- Explicit and predictable
- Single API

**Cons:**
- Verbose for typical use cases
- User must track units manually
- Easy to make mistakes

### Alternative C: Automatic Only

```cpp
material->setTexture("u_DiffuseMap", texture);  // Always auto-assign
```

**Pros:**
- Simple and convenient
- No unit management

**Cons:**
- No control over unit assignment
- Can't match shader expectations (some shaders expect specific units)
- Performance: can't pack commonly-used textures to low units

**Decision:** **Manual + Automatic** because:
1. Default case (automatic) is convenient
2. Advanced case (manual) provides control
3. Specialized materials (PhongMaterial) can hardcode units internally
4. Validation prevents unit conflicts regardless of method

---

## Summary Table

| Design Aspect | Chosen Solution | Key Benefit | Main Trade-off |
|---------------|----------------|-------------|----------------|
| Shader Ownership | Raw Pointer | Clear non-owning semantics | No lifetime tracking |
| Texture Ownership | Shared Pointer | Safe sharing between materials | Reference counting overhead |
| Uniform Storage | std::variant | Type safety + performance | Limited to predefined types |
| Texture Lookup | Vector + Map | Fast iteration + O(1) lookup | Dual data structures |
| Bind Pattern | Explicit bind/unbind | Clear and debuggable | Not exception-safe by default |
| Property Upload | Lazy on bind() | Simplicity | Re-uploads unchanged properties |
| Specialized Materials | Inheritance | Type-safe convenient APIs | Class proliferation |
| Texture Units | Manual + Automatic | Flexibility | Slightly more complex API |

## Conclusion

The chosen design prioritizes:
1. **Type Safety** - Compile-time checking prevents common errors
2. **Performance** - Cache-friendly data structures for render loop
3. **Clarity** - Explicit APIs over hidden magic
4. **Pragmatism** - Simple solutions over premature optimization
5. **Extensibility** - Virtual methods and inheritance for custom materials

Each alternative was carefully considered, and the trade-offs documented. The design can evolve as profiling reveals bottlenecks or new requirements emerge.

**When to Reconsider:**
- **Shader Ownership:** If shaders need dynamic reload → use weak_ptr
- **Uniform Storage:** If custom types needed → extend variant or use std::any
- **Property Upload:** If profiling shows bottleneck → add dirty tracking
- **Specialized Materials:** If data-driven needed → add template system

The architecture is designed to accommodate these changes without major refactoring.
