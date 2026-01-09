# Quick Reference Guide

## TL;DR - The Essential Info

### Class Names
- **Mesh** - Geometry data (vertices, indices, colors, UVs)
- **Renderable** - Complete drawable object (mesh + textures + shaders + GPU buffers)

Alternative names provided: `Model`, `RenderObject`, `Drawable`

---

## Key Design Decisions

| Aspect | Mesh | Renderable |
|--------|------|------------|
| **Semantics** | Value type (copyable) | Reference type (move-only) |
| **Contains** | CPU geometry data | GPU resources + rendering state |
| **Ownership** | Owned by value | Shared via shared_ptr |
| **Cost** | Cheap to copy (data) | Expensive (GPU resources) |
| **Purpose** | Pure data | Presentation layer |

---

## Quick Start Code

### Creating a Mesh

```cpp
// Option 1: Use factory
Mesh cube = MeshFactory::createCube(2.0f);

// Option 2: Build manually
Mesh triangle;
triangle.addVertex({-0.5f, 0.0f, 0.0f}, {1,0,0}, {0,0});
triangle.addVertex({ 0.5f, 0.0f, 0.0f}, {0,1,0}, {1,0});
triangle.addVertex({ 0.0f, 0.5f, 0.0f}, {0,0,1}, {0.5f,1});
triangle.addTriangle(0, 1, 2);
triangle.computeFlatNormals();
```

### Creating a Renderable

```cpp
// Create mesh and wrap in shared_ptr
auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());

// Create renderable
Renderable obj(mesh);
obj.setShader(myShader);
obj.setTexture(myTexture);
obj.uploadToGPU();

// Render
if (obj.isReadyToRender()) {
    obj.render();
}
```

### Instancing Pattern (Share Mesh)

```cpp
auto sharedMesh = std::make_shared<Mesh>(MeshFactory::createSphere());

// Create multiple objects with same geometry
Renderable obj1(sharedMesh);
Renderable obj2(sharedMesh);
Renderable obj3(sharedMesh);
// Memory efficient - one mesh, multiple renderables
```

---

## Common Patterns

### Pattern: Static Object

```cpp
auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());
Renderable obj(mesh);
obj.uploadToGPU();

// Render loop
while (running) {
    obj.render(); // No re-upload needed
}
```

### Pattern: Dynamic/Animated Object

```cpp
auto mesh = std::make_shared<Mesh>(...);
Renderable obj(mesh);

// Update loop
while (running) {
    // Modify mesh data
    mesh->getVertices()[i].y += deltaY;

    // Mark dirty and re-upload
    obj.invalidateGPUData();
    obj.uploadToGPU();

    obj.render();
}
```

### Pattern: Multiple Materials, Same Geometry

```cpp
auto mesh = std::make_shared<Mesh>(...);

Renderable woodObj(mesh);
woodObj.setTexture(woodTexture);

Renderable metalObj(mesh);
metalObj.setTexture(metalTexture);

// Same geometry, different appearance
```

---

## Method Cheat Sheet

### Mesh - Essential Methods

```cpp
// Construction
Mesh()
Mesh(vertexCount, indexCount)

// Building
void addVertex(pos)
void addVertex(pos, color)
void addVertex(pos, color, texCoord)
void addVertex(pos, color, texCoord, normal)
void addIndex(index)
void addTriangle(i0, i1, i2)

// Access (const and non-const versions)
const std::vector<Vec3>& getVertices() const
std::vector<Vec3>& getVertices()
// ... similar for indices, colors, texCoords, normals

// Utilities
bool isValid() const
bool validate() const
size_t getVertexCount() const
size_t getIndexCount() const
void computeFlatNormals()
void computeSmoothNormals()
void clear()
void reset()
```

### Renderable - Essential Methods

```cpp
// Construction
Renderable()
Renderable(meshPtr)
Renderable(meshPtr, shaderPtr, texturePtr)

// Resource Management
void setMesh(meshPtr)
void setShader(shaderPtr)
void setTexture(texturePtr)
void setTexture(name, texturePtr)  // Named texture
void setMaterial(materialPtr)

// GPU Operations
void uploadToGPU()
void invalidateGPUData()
void releaseGPUResources()
bool isUploadedToGPU() const

// Rendering
void render() const
void render(shader) const
bool isReadyToRender() const

// State
void setEnabled(bool)
void setCastsShadows(bool)
void setReceivesShadows(bool)
bool isEnabled() const
```

---

## Memory Management Rules

### Do This ✅

```cpp
// Use smart pointers for resources
auto mesh = std::make_shared<Mesh>(...);
auto shader = std::make_shared<Shader>(...);
auto texture = std::make_shared<Texture>(...);

// Move renderables (don't copy)
Renderable obj1(mesh);
Renderable obj2 = std::move(obj1);  // ✅ Move is fine

// Share expensive resources
std::vector<Renderable> objects;
for (int i = 0; i < 100; ++i) {
    objects.emplace_back(mesh);  // ✅ All share same mesh
}
```

### Don't Do This ❌

```cpp
// Don't use raw pointers for ownership
Mesh* mesh = new Mesh(...);  // ❌ Use shared_ptr instead
delete mesh;

// Don't copy Renderables
Renderable obj1(mesh);
Renderable obj2 = obj1;  // ❌ Compile error (deleted)

// Don't forget to upload to GPU
Renderable obj(mesh);
obj.render();  // ❌ Not uploaded yet!
```

---

## Validation Checklist

Before rendering, ensure:
- ✅ Mesh is valid: `mesh->validate()`
- ✅ Mesh has vertices and indices
- ✅ Data uploaded to GPU: `renderable.uploadToGPU()`
- ✅ Shader is set: `renderable.setShader(...)`
- ✅ Renderable is ready: `renderable.isReadyToRender()`

```cpp
// Safe rendering pattern
if (mesh->validate() && renderable.isReadyToRender()) {
    renderable.render();
} else {
    // Handle error
}
```

---

## Performance Tips

1. **Pre-allocate**: Use `mesh.reserve(vertexCount, indexCount)` before building
2. **Share resources**: Use shared_ptr for meshes, textures, shaders
3. **Batch uploads**: Modify mesh, then upload once (not per-vertex)
4. **Move, don't copy**: Use `std::move()` for Renderables
5. **Validate once**: Check validity before upload, not every frame

---

## Common Pitfalls

### Pitfall 1: Forgetting to Upload

```cpp
// Wrong ❌
Renderable obj(mesh);
obj.render();  // Crashes or undefined behavior

// Right ✅
Renderable obj(mesh);
obj.uploadToGPU();
obj.render();
```

### Pitfall 2: Inconsistent Attributes

```cpp
// Wrong ❌
mesh.addVertex({0,0,0}, {1,0,0});  // Has color
mesh.addVertex({1,0,0});            // No color - inconsistent!

// Right ✅
mesh.addVertex({0,0,0}, {1,0,0});
mesh.addVertex({1,0,0}, {0,1,0});  // All have colors
```

### Pitfall 3: Modifying Shared Mesh

```cpp
auto mesh = std::make_shared<Mesh>(...);
Renderable obj1(mesh);
Renderable obj2(mesh);

// Careful! This affects both obj1 and obj2
mesh->getVertices()[0].y += 1.0f;

// Need to re-upload both
obj1.uploadToGPU();
obj2.uploadToGPU();
```

---

## Architecture Summary

```
┌──────────────────────────────────────┐
│          Your Application            │
└──────────────────────────────────────┘
                  │
                  ├─→ Create Mesh (CPU data)
                  │   ├─ vertices, indices
                  │   ├─ colors, texCoords
                  │   └─ normals
                  │
                  ├─→ Create Renderable
                  │   ├─ References Mesh (shared_ptr)
                  │   ├─ Owns GPU buffers (unique_ptr)
                  │   ├─ References Shader (shared_ptr)
                  │   └─ References Texture (shared_ptr)
                  │
                  └─→ Render Loop
                      ├─ uploadToGPU() [once or on change]
                      └─ render() [every frame]
```

---

## File Summary

- **Mesh.h** - Mesh class definition
- **Mesh.cpp** - Mesh implementation + factory functions
- **Renderable.h** - Renderable class definition
- **ExampleUsage.cpp** - Complete usage examples
- **ARCHITECTURE.md** - Detailed design document
- **QUICK_REFERENCE.md** - This file

---

## When to Use What

| Use Case | Use This |
|----------|----------|
| Store geometry data | `Mesh` |
| Draw an object | `Renderable` |
| Share geometry across objects | `shared_ptr<Mesh>` |
| Create primitives | `MeshFactory::createXXX()` |
| Load from file | Custom loader → `Mesh` |
| Apply material | `Renderable::setTexture/setShader` |
| Animate vertices | Modify `Mesh`, then `uploadToGPU()` |
| Render many similar objects | Share `Mesh`, unique `Renderable` per instance |

---

## Next Steps

1. Implement platform-specific GPU buffer classes (VertexBuffer, IndexBuffer)
2. Implement Shader and Texture classes
3. Add Material system for complex rendering
4. Integrate with your rendering API (OpenGL/Vulkan/DirectX)
5. Add asset loading (Assimp, tinyobjloader, etc.)
6. Extend with animation, LOD, or other features as needed

**Remember**: This architecture is a foundation. Extend it based on your specific needs, but maintain the core separation between data (Mesh) and presentation (Renderable).
