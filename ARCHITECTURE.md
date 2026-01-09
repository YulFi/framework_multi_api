# Graphics Class Architecture Design Document

## Overview

This document explains the architectural design of the `Mesh` and `Renderable` classes for a modern C++ graphics application. The design follows industry best practices, SOLID principles, and leverages modern C++ features for safety and performance.

---

## Class Naming Conventions

### Primary Classes

1. **Mesh** - Universally recognized name for geometry data
2. **Renderable** - Chosen as the primary name for the second class

### Alternative Names (Type Aliases Provided)

- **Model** - Common in game engines (Unity, Unreal)
- **RenderObject** - Common in rendering engines
- **Drawable** - Common in scene graphs (Qt, JavaFX)

All alternatives are provided as type aliases, allowing teams to use their preferred terminology while maintaining a consistent implementation.

---

## Core Design Principles

### 1. Separation of Concerns

**Mesh (Data Layer)**
- Pure geometry data (vertices, indices, colors, UVs, normals)
- No GPU-specific code or dependencies
- Lightweight, copyable, value semantics
- Can be serialized, transmitted, or stored easily

**Renderable (Presentation Layer)**
- Combines geometry with rendering resources
- Manages GPU buffers, textures, shaders
- Handles rendering state and draw calls
- Move-only to prevent accidental resource duplication

**Rationale**: This separation allows:
- Mesh reuse across multiple renderables (instancing)
- Independent modification of geometry vs rendering properties
- Easy testing (Mesh doesn't require GPU context)
- Clear ownership semantics

### 2. Value vs Reference Semantics

**Mesh: Value Semantics**
```cpp
Mesh mesh1 = MeshFactory::createCube();
Mesh mesh2 = mesh1; // Deep copy - safe and expected
```

- Copyable and moveable (Rule of Zero)
- Behaves like standard containers
- Suitable for storage in vectors, maps, etc.

**Renderable: Reference Semantics (Move-Only)**
```cpp
Renderable r1(mesh);
Renderable r2 = std::move(r1); // Move is OK
// Renderable r3 = r1; // Compile error - can't copy GPU resources
```

- Move-only (deleted copy constructor/assignment)
- Prevents accidental duplication of GPU resources
- Explicit about resource ownership transfer

**Rationale**:
- Value semantics for data is intuitive and safe
- Move-only prevents resource management bugs
- Follows std::unique_ptr model for unique resources

### 3. Resource Ownership Model

```
┌─────────────┐
│ Renderable  │ (unique owner)
└─────────────┘
      │
      ├─→ shared_ptr<Mesh>        (shared - cheap to share)
      ├─→ shared_ptr<Shader>      (shared - expensive to compile)
      ├─→ shared_ptr<Texture>     (shared - large memory footprint)
      ├─→ unique_ptr<VertexBuffer> (unique - specific to this object)
      └─→ unique_ptr<IndexBuffer>  (unique - specific to this object)
```

**Shared Resources** (shared_ptr):
- Mesh data (can be reused for instancing)
- Shaders (expensive compilation, shared across many objects)
- Textures (large memory, shared across materials)
- Materials (can be shared or unique)

**Unique Resources** (unique_ptr):
- GPU vertex buffers (uploaded per-renderable)
- GPU index buffers (uploaded per-renderable)
- Vertex array objects (OpenGL-specific state)

**Rationale**:
- Shared resources reduce memory footprint
- Unique ownership prevents double-free bugs
- Smart pointers ensure automatic cleanup (RAII)
- Clear ownership hierarchy aids debugging

---

## Memory Management Strategy

### RAII (Resource Acquisition Is Initialization)

All resources are managed through RAII:
```cpp
{
    Renderable obj(mesh);
    obj.uploadToGPU(); // Allocates GPU memory

    // Use object...

} // Destructor automatically frees GPU memory - no manual cleanup needed
```

### Benefits:
- **Exception Safety**: Resources freed even if exceptions occur
- **Automatic Cleanup**: No manual delete/free calls
- **Leak Prevention**: Impossible to forget cleanup
- **Clear Lifetimes**: Resource scope matches object scope

### Smart Pointer Guidelines

1. **Use shared_ptr when**:
   - Resource is expensive to create/copy
   - Multiple objects need to access the same resource
   - Lifetime is complex or shared

2. **Use unique_ptr when**:
   - Resource belongs to exactly one owner
   - Transfer of ownership may be needed (move)
   - Performance is critical (less overhead than shared_ptr)

3. **Never use raw pointers for ownership**:
   - Raw pointers only for non-owning references
   - Prevents memory leaks and double-frees

---

## API Design Patterns

### 1. Const Correctness

```cpp
// Read-only access
const std::vector<Vec3>& getVertices() const noexcept;

// Mutable access (for building/modifying)
std::vector<Vec3>& getVertices() noexcept;
```

**Benefits**:
- Compiler-enforced immutability
- Self-documenting (intent is clear)
- Enables optimizations
- Prevents accidental modifications

### 2. [[nodiscard]] Attribute

```cpp
[[nodiscard]] bool isValid() const noexcept;
[[nodiscard]] size_t getVertexCount() const noexcept;
```

**Benefits**:
- Compiler warns if return value is ignored
- Prevents silent errors (forgetting to check validation)
- Modern C++ best practice (C++17+)

### 3. noexcept Specification

```cpp
void clear() noexcept;
bool isEnabled() const noexcept;
```

**Benefits**:
- Documents exception guarantee
- Enables compiler optimizations
- Required for move constructors/assignment
- Helps with exception safety reasoning

### 4. Explicit Constructors

```cpp
explicit Renderable(MeshPtr mesh);
```

**Benefits**:
- Prevents unintended implicit conversions
- Makes code intent explicit
- Reduces subtle bugs

---

## Design Patterns Applied

### 1. Factory Pattern

```cpp
namespace MeshFactory {
    Mesh createCube(float size = 1.0f);
    Mesh createSphere(float radius = 1.0f, uint32_t segments = 32);
    Mesh createPlane(float width = 1.0f, float height = 1.0f);
}
```

**Benefits**:
- Encapsulates complex construction logic
- Provides pre-built primitives
- Extensible for new shapes
- Clear, expressive API

### 2. Builder Pattern (Implicit)

```cpp
Mesh mesh;
mesh.addVertex({0, 0, 0});
mesh.addVertex({1, 0, 0});
mesh.addTriangle(0, 1, 2);
```

**Benefits**:
- Fluent, incremental construction
- Step-by-step mesh building
- Natural for procedural generation

### 3. Flyweight Pattern (Implicit)

```cpp
auto sharedMesh = std::make_shared<Mesh>(...);
Renderable obj1(sharedMesh);
Renderable obj2(sharedMesh); // Shares geometry
```

**Benefits**:
- Memory efficiency (one mesh, many instances)
- Enables GPU instancing
- Natural with shared_ptr

### 4. Template Method Pattern

```cpp
void render() const {
    bind();           // Setup state
    // ... draw call ...
    unbind();         // Cleanup
}
```

**Benefits**:
- Consistent rendering flow
- Extensibility through virtual methods
- Encapsulates rendering protocol

---

## Performance Considerations

### 1. Cache Locality

```cpp
// All vertices stored contiguously
std::vector<Vec3> vertices_;
```

**Benefits**:
- Sequential memory access
- CPU cache-friendly
- Faster iteration

### 2. Move Semantics

```cpp
Renderable obj = createRenderable(); // Move, not copy
renderables.push_back(std::move(obj)); // Explicit move
```

**Benefits**:
- Avoids expensive copies
- Zero-cost transfers
- Modern C++ idiom

### 3. Reserve Strategy

```cpp
mesh.reserve(1000, 3000); // Pre-allocate before building
```

**Benefits**:
- Prevents repeated reallocations
- Reduces memory fragmentation
- Improves construction performance

### 4. Lazy GPU Upload

```cpp
renderable.setMesh(newMesh);      // Just updates pointer
// ... more modifications ...
renderable.uploadToGPU();         // Single upload at end
```

**Benefits**:
- Batch multiple changes
- Reduces GPU synchronization points
- Explicit control over expensive operations

---

## Extensibility Points

### 1. Adding New Vertex Attributes

```cpp
// Easy to extend Mesh with new attributes:
class Mesh {
    std::vector<Vec3> vertices_;
    std::vector<Vec3> tangents_;    // Add for normal mapping
    std::vector<Vec3> bitangents_;  // Add for normal mapping
    std::vector<float> weights_;    // Add for skeletal animation
};
```

### 2. Material System

```cpp
class Material {
    ShaderPtr shader_;
    std::unordered_map<std::string, TexturePtr> textures_;
    std::unordered_map<std::string, ShaderParameter> parameters_;
};
```

### 3. LOD (Level of Detail)

```cpp
class Renderable {
    std::vector<MeshPtr> lodLevels_;
    MeshPtr selectLOD(float distance) const;
};
```

### 4. Animation Support

```cpp
class AnimatedRenderable : public Renderable {
    std::vector<Mesh> animationFrames_;
    void updateAnimation(float deltaTime);
};
```

---

## Exception Safety Guarantees

### Strong Guarantee

Most operations provide the strong guarantee (commit-or-rollback):

```cpp
void setMesh(MeshPtr mesh) {
    mesh_ = std::move(mesh);  // No-throw
    invalidateGPUData();       // No-throw
}
```

### Basic Guarantee

GPU operations provide basic guarantee (valid but unspecified state):

```cpp
void uploadToGPU() {
    // If upload fails, GPU buffers may be partially created
    // But destructor will still clean up properly
}
```

### No-Throw Guarantee

Queries and state checks are no-throw:

```cpp
bool isValid() const noexcept;
size_t getVertexCount() const noexcept;
```

---

## Testing Strategy

### Unit Testing

**Mesh Testing**:
- Test factory functions produce valid geometry
- Test attribute consistency validation
- Test normal computation correctness
- Test copy/move semantics

**Renderable Testing**:
- Test resource management (no leaks)
- Test move semantics
- Test state transitions
- Mock GPU calls for pure CPU testing

### Integration Testing

- Test Mesh → GPU pipeline
- Test rendering loop with multiple objects
- Test resource sharing scenarios

### Performance Testing

- Benchmark mesh construction
- Benchmark GPU upload times
- Profile memory usage with sharing

---

## Common Usage Patterns

### Pattern 1: Static Geometry

```cpp
auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());
Renderable obj(mesh);
obj.uploadToGPU();
// Render many times without re-uploading
```

### Pattern 2: Dynamic Geometry

```cpp
auto mesh = std::make_shared<Mesh>(...);
Renderable obj(mesh);

// In update loop:
mesh->getVertices()[i].y += deltaY;  // Modify
obj.invalidateGPUData();
obj.uploadToGPU();  // Re-upload changed data
```

### Pattern 3: Instancing

```cpp
auto sharedMesh = std::make_shared<Mesh>(...);

for (int i = 0; i < 1000; ++i) {
    Renderable instance(sharedMesh);
    // Each instance has unique position/material but shares geometry
}
```

### Pattern 4: Material Variants

```cpp
auto mesh = std::make_shared<Mesh>(...);

Renderable wood(mesh);
wood.setTexture(woodTexture);

Renderable metal(mesh);
metal.setTexture(metalTexture);
// Same geometry, different appearance
```

---

## Trade-offs and Alternatives

### Design Decision: Mesh as Value Type

**Chosen**: Value semantics (copyable)

**Alternative**: Reference semantics (always use shared_ptr)
- **Pro**: Prevents accidental copies
- **Con**: More verbose, always heap-allocated
- **Verdict**: Value semantics more intuitive for data

### Design Decision: Explicit GPU Upload

**Chosen**: Explicit `uploadToGPU()` call

**Alternative**: Automatic upload on first `render()`
- **Pro**: More convenient, less to remember
- **Con**: Hidden performance cost, less control
- **Verdict**: Explicit better for performance-critical code

### Design Decision: Shared vs Unique Mesh

**Chosen**: Renderable stores `shared_ptr<Mesh>`

**Alternative**: Renderable stores `unique_ptr<Mesh>`
- **Pro**: Clearer ownership
- **Con**: Can't share geometry (less efficient)
- **Verdict**: Sharing is too valuable for instancing

### Design Decision: Platform Abstraction

**Chosen**: Abstract GPU types (VertexBuffer, etc.)

**Alternative**: Direct API types (VkBuffer, GLuint, etc.)
- **Pro**: More direct, less indirection
- **Con**: Platform-specific, harder to port
- **Verdict**: Abstraction enables multi-API support

---

## Migration and Integration Guide

### Integrating with OpenGL

```cpp
class GLVertexBuffer : public VertexBuffer {
    GLuint vbo_ = 0;
public:
    void upload(const void* data, size_t size) override {
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }
    ~GLVertexBuffer() { glDeleteBuffers(1, &vbo_); }
};
```

### Integrating with Vulkan

```cpp
class VkVertexBuffer : public VertexBuffer {
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    // ... Vulkan-specific implementation
};
```

### Asset Loading Integration

```cpp
Mesh loadMeshFromFile(const std::string& path) {
    // Use Assimp, tinyobjloader, or custom format
    Mesh mesh;
    // ... load data into mesh
    mesh.computeSmoothNormals();
    return mesh;
}
```

---

## Future Enhancements

1. **GPU Instancing Support**: Add instance data buffers
2. **Mesh Compression**: Store compressed vertex data
3. **Streaming**: Load mesh data on-demand
4. **Ray Tracing**: BVH construction for acceleration
5. **Compute Shaders**: GPU-side mesh manipulation
6. **Multi-threading**: Thread-safe resource management

---

## Conclusion

This architecture provides:
- **Safety**: RAII, smart pointers, const-correctness
- **Performance**: Move semantics, cache locality, resource sharing
- **Flexibility**: Clear separation, extensibility points
- **Maintainability**: Self-documenting, industry-standard patterns
- **Correctness**: Validation, exception safety, type safety

The design balances modern C++ best practices with practical graphics programming needs, creating a foundation that can scale from simple applications to complex rendering engines.
