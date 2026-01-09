# Graphics Classes: Mesh and Renderable

## Overview

This repository contains a professionally designed, modern C++ architecture for handling 3D graphics objects in rendering applications. The design separates geometry data (Mesh) from rendering resources (Renderable), following industry best practices and modern C++ standards.

---

## Files Included

### Core Headers and Implementation
- **Mesh.h** (8.1 KB) - Mesh class definition with full documentation
- **Mesh.cpp** (15 KB) - Implementation including factory functions for primitives
- **Renderable.h** (13 KB) - Renderable class definition with comprehensive API

### Documentation
- **ARCHITECTURE.md** (15 KB) - Complete architectural design document
  - Design principles and rationale
  - Memory management strategy
  - API design patterns
  - Performance considerations
  - Extensibility guide

- **QUICK_REFERENCE.md** (8.9 KB) - Concise quick-start guide
  - Essential patterns and code snippets
  - Common pitfalls and solutions
  - Method cheat sheets

- **ExampleUsage.cpp** (14 KB) - Comprehensive usage examples
  - 9 complete examples covering all use cases
  - Best practices demonstrations
  - Error handling patterns

---

## Key Features

### Mesh Class
- ✅ Pure geometry data container (vertices, indices, colors, UVs, normals)
- ✅ Value semantics (fully copyable and moveable)
- ✅ Built-in validation and consistency checking
- ✅ Automatic normal computation (flat and smooth)
- ✅ Factory functions for common primitives (cube, sphere, plane, cylinder)
- ✅ No GPU dependencies - pure data representation

### Renderable Class
- ✅ Complete rendering object (mesh + textures + shaders + GPU buffers)
- ✅ RAII-based resource management (automatic cleanup)
- ✅ Move-only semantics (prevents accidental resource duplication)
- ✅ Smart pointer-based resource sharing
- ✅ Explicit GPU upload control with dirty tracking
- ✅ Support for multiple textures and materials
- ✅ Rendering state management (enabled, shadows, etc.)

---

## Quick Start

### 1. Create a Mesh

```cpp
#include "Mesh.h"

using namespace Graphics;

// Option A: Use factory
Mesh cube = MeshFactory::createCube(2.0f);

// Option B: Build manually
Mesh triangle;
triangle.addVertex({-0.5f, 0.0f, 0.0f}, {1,0,0}, {0,0});
triangle.addVertex({ 0.5f, 0.0f, 0.0f}, {0,1,0}, {1,0});
triangle.addVertex({ 0.0f, 0.5f, 0.0f}, {0,0,1}, {0.5f,1});
triangle.addTriangle(0, 1, 2);
triangle.computeFlatNormals();

// Validate
if (triangle.validate()) {
    // Mesh is ready to use
}
```

### 2. Create a Renderable

```cpp
#include "Renderable.h"

// Create shared mesh (can be reused across multiple renderables)
auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());

// Create renderable
Renderable obj(mesh);
obj.setShader(myShader);      // Set shader program
obj.setTexture(myTexture);    // Set primary texture
obj.uploadToGPU();            // Upload to GPU buffers

// Render
if (obj.isReadyToRender()) {
    obj.render();
}
```

### 3. Share Geometry (Instancing)

```cpp
// One mesh, multiple objects (memory efficient)
auto sharedMesh = std::make_shared<Mesh>(MeshFactory::createSphere());

Renderable obj1(sharedMesh);
obj1.setTexture(texture1);

Renderable obj2(sharedMesh);
obj2.setTexture(texture2);

Renderable obj3(sharedMesh);
obj3.setTexture(texture3);

// All three share the same geometry but can have different materials
```

---

## Architecture Highlights

### Separation of Concerns

```
┌─────────────┐        ┌──────────────────┐
│    Mesh     │────────│   Renderable     │
│ (Data)      │        │ (Presentation)   │
├─────────────┤        ├──────────────────┤
│ • Vertices  │        │ • Mesh ref       │
│ • Indices   │        │ • Textures       │
│ • Colors    │        │ • Shaders        │
│ • UVs       │        │ • GPU buffers    │
│ • Normals   │        │ • State          │
└─────────────┘        └──────────────────┘
   Copyable              Move-only
   No GPU code           GPU resources
```

### Resource Ownership Model

```
Renderable (unique owner)
├─→ shared_ptr<Mesh>         (shared - reusable geometry)
├─→ shared_ptr<Shader>       (shared - expensive to compile)
├─→ shared_ptr<Texture>      (shared - large memory)
├─→ unique_ptr<VertexBuffer> (unique - per-object GPU buffer)
└─→ unique_ptr<IndexBuffer>  (unique - per-object GPU buffer)
```

### Memory Safety

- **RAII everywhere**: Automatic resource cleanup, no manual delete
- **Smart pointers**: Clear ownership, no memory leaks
- **Move semantics**: Efficient transfers, no expensive copies
- **Const correctness**: Compiler-enforced immutability where appropriate
- **Exception safety**: Strong guarantees on most operations

---

## Design Principles Applied

1. **SOLID Principles**
   - Single Responsibility: Mesh is data, Renderable is presentation
   - Open/Closed: Extensible through inheritance and composition
   - Liskov Substitution: Clean interfaces, proper polymorphism
   - Interface Segregation: Minimal, focused APIs
   - Dependency Inversion: Depends on abstractions (Texture, Shader interfaces)

2. **Modern C++ Best Practices**
   - Rule of Zero/Five followed correctly
   - Smart pointers for all ownership
   - Move semantics for efficiency
   - [[nodiscard]] for error checking
   - noexcept specifications
   - Const correctness throughout

3. **Design Patterns**
   - Factory Pattern: MeshFactory for primitive creation
   - RAII: Automatic resource management
   - Flyweight: Resource sharing via shared_ptr
   - Template Method: Consistent rendering flow

---

## Integration Guide

### Step 1: Implement Platform-Specific GPU Resources

You need to implement these abstract classes for your rendering API:

```cpp
// Example for OpenGL
class GLVertexBuffer : public VertexBuffer {
    GLuint vbo_;
public:
    void upload(const void* data, size_t size) override;
    void bind() const override;
    ~GLVertexBuffer() override;
};

// Example for Vulkan
class VkVertexBuffer : public VertexBuffer {
    VkBuffer buffer_;
    VkDeviceMemory memory_;
public:
    void upload(const void* data, size_t size) override;
    void bind() const override;
    ~VkVertexBuffer() override;
};
```

### Step 2: Implement Shader and Texture Classes

```cpp
class Shader {
public:
    virtual void bind() const = 0;
    virtual void setUniform(const std::string& name, const void* data) = 0;
    virtual ~Shader() = default;
};

class Texture {
public:
    virtual void bind(uint32_t slot = 0) const = 0;
    virtual ~Texture() = default;
};
```

### Step 3: Integrate with Your Render Loop

```cpp
void renderScene(const std::vector<Renderable>& objects) {
    for (const auto& obj : objects) {
        if (!obj.isEnabled() || !obj.isReadyToRender()) {
            continue;
        }

        // Set per-object transforms
        shader.setUniform("model", obj.getTransform());

        // Render
        obj.render();
    }
}
```

---

## Compilation

### Requirements
- C++17 or later
- Standard library (no external dependencies for core classes)

### Build with g++

```bash
g++ -std=c++17 -o example ExampleUsage.cpp Mesh.cpp -I.
```

### Build with CMake

```cmake
add_library(graphics_core Mesh.cpp)
target_compile_features(graphics_core PUBLIC cxx_std_17)
target_include_directories(graphics_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(example ExampleUsage.cpp)
target_link_libraries(example PRIVATE graphics_core)
```

### Build with Visual Studio

Add to your .vcxproj or create a new C++ project:
- Add Mesh.h, Mesh.cpp, Renderable.h to project
- Set C++ Standard to C++17 or later
- Build and run ExampleUsage.cpp

---

## Testing

Recommended tests to implement:

### Unit Tests (Mesh)
- ✅ Vertex addition and retrieval
- ✅ Index addition and validation
- ✅ Attribute consistency checking
- ✅ Normal computation correctness
- ✅ Factory function output validation
- ✅ Copy and move semantics

### Unit Tests (Renderable)
- ✅ Resource management (no leaks)
- ✅ Move semantics
- ✅ State transitions (enabled/disabled)
- ✅ GPU upload triggering
- ✅ Validation logic

### Integration Tests
- ✅ Mesh to GPU pipeline
- ✅ Multi-object rendering
- ✅ Resource sharing scenarios
- ✅ Dynamic mesh updates

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Mesh creation | O(n) | n = vertices |
| Mesh copy | O(n) | Deep copy of all data |
| Mesh move | O(1) | Move semantics |
| addVertex | O(1) amortized | May reallocate |
| addTriangle | O(1) amortized | May reallocate |
| GPU upload | O(n) | Data transfer to GPU |
| Renderable move | O(1) | Transfer ownership |
| render() | O(1) | Draw call overhead |

### Memory Footprint

**Mesh** (per vertex):
- Position: 12 bytes (3 floats)
- Color: 12 bytes (3 floats)
- TexCoord: 8 bytes (2 floats)
- Normal: 12 bytes (3 floats)
- **Total: ~44 bytes/vertex** (when all attributes present)

**Renderable**:
- Pointers: ~64 bytes (shared_ptr overhead)
- GPU buffers: Duplicates mesh data on GPU
- State flags: ~16 bytes

---

## Extension Points

### Add Custom Vertex Attributes

```cpp
class Mesh {
    std::vector<Vec3> tangents_;
    std::vector<Vec3> bitangents_;
    std::vector<float> weights_;
    std::vector<uint32_t> boneIndices_;
};
```

### Add LOD Support

```cpp
class Renderable {
    std::vector<MeshPtr> lodLevels_;

    MeshPtr selectLOD(float distance) const {
        // Select appropriate LOD based on distance
    }
};
```

### Add Animation

```cpp
class AnimatedRenderable : public Renderable {
    std::vector<Mesh> keyframes_;
    float currentTime_;

    void updateAnimation(float deltaTime);
    void interpolateMesh();
};
```

### Add Material System

```cpp
struct Material {
    std::unordered_map<std::string, TexturePtr> textures;
    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, Vec3> vec3Params;
};
```

---

## Common Use Cases

### Static Scene Objects
```cpp
auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());
Renderable building(mesh);
building.uploadToGPU();
// Render every frame without re-uploading
```

### Procedurally Generated Terrain
```cpp
Mesh terrain;
for (int z = 0; z < size; ++z) {
    for (int x = 0; x < size; ++x) {
        float height = getHeight(x, z);
        terrain.addVertex({x, height, z}, getColor(height), {x, z});
    }
}
generateIndices(terrain);
terrain.computeSmoothNormals();
```

### Particle Systems
```cpp
auto sphereMesh = std::make_shared<Mesh>(MeshFactory::createSphere(0.1f));
std::vector<Renderable> particles;

for (int i = 0; i < 1000; ++i) {
    particles.emplace_back(sphereMesh); // Share mesh
    // Set unique position/color per particle
}
```

### Dynamic Deformation
```cpp
auto mesh = std::make_shared<Mesh>(...);
Renderable character(mesh);

// Animation loop
for (auto& vertex : mesh->getVertices()) {
    vertex.y += sin(time + vertex.x) * 0.1f;
}
character.invalidateGPUData();
character.uploadToGPU();
```

---

## Troubleshooting

### "Mesh validation failed"
- Ensure vertices and indices are both present
- Check that all attribute arrays have same size as vertices (or are empty)
- Verify indices don't exceed vertex count

### "Not ready to render"
- Verify mesh is set and valid
- Ensure `uploadToGPU()` was called
- Check that shader is set
- Use `renderable.getDebugInfo()` for details

### Memory leaks
- Ensure you're using smart pointers correctly
- Don't mix raw pointers with ownership semantics
- Let RAII handle cleanup automatically

### Performance issues
- Use `mesh.reserve()` before building large meshes
- Share meshes via shared_ptr for instancing
- Batch GPU uploads (modify mesh, then upload once)
- Profile before optimizing

---

## Future Enhancements

Possible additions to consider:
- GPU instancing with instance buffers
- Mesh compression (quantization, etc.)
- Asynchronous asset loading
- BVH/octree for ray tracing
- Multi-threaded mesh processing
- Streaming for large worlds

---

## License and Usage

These classes are designed as educational reference and starting point for graphics applications. Feel free to adapt, extend, and integrate into your projects. The architecture is intentionally API-agnostic to support OpenGL, Vulkan, DirectX, or any other rendering backend.

---

## Support and Documentation

- **ARCHITECTURE.md** - Deep dive into design decisions
- **QUICK_REFERENCE.md** - Fast lookup for common patterns
- **ExampleUsage.cpp** - 9 complete working examples

For questions or issues, refer to the comprehensive documentation files included.

---

## Summary

This architecture provides a solid foundation for graphics applications by:
- ✅ Separating data from presentation
- ✅ Ensuring memory safety through RAII and smart pointers
- ✅ Enabling resource sharing for performance
- ✅ Following modern C++ best practices
- ✅ Providing clear, documented APIs
- ✅ Supporting extensibility without modification

Start with the examples, review the architecture document, and extend as needed for your specific use case.
