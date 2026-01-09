# RenderMesh Architecture Documentation

## Overview

`RenderMesh` is a GPU-side mesh representation that bridges CPU-side `Mesh` data with the rendering API. It encapsulates vertex array objects (VAO), vertex buffers (VBO), and index buffers (IBO) while providing a clean, easy-to-use interface for rendering.

## Design Principles

### 1. Clear Separation of Concerns

**CPU-side (Mesh)**
- Value-semantic container
- Stores geometry data in separate arrays
- No GPU knowledge
- Copyable and movable
- Lives in application/game logic layer

**GPU-side (RenderMesh)**
- Resource-semantic wrapper
- Manages GPU buffers
- Handles data upload and interleaving
- Non-copyable (GPU resources are expensive)
- Lives in rendering layer

### 2. RAII Resource Management

```cpp
{
    RenderMesh gpuMesh(cpuMesh, renderer, BufferUsage::Static);

    // Use gpuMesh...

} // GPU resources (VAO, VBO, IBO) automatically released here
```

**Benefits:**
- Exception-safe resource cleanup
- No manual memory management
- Impossible to leak GPU resources
- Clear ownership semantics via `unique_ptr`

### 3. Move-Only Semantics

```cpp
class RenderMesh {
    RenderMesh(const RenderMesh&) = delete;            // Non-copyable
    RenderMesh& operator=(const RenderMesh&) = delete;

    RenderMesh(RenderMesh&&) noexcept = default;       // Movable
    RenderMesh& operator=(RenderMesh&&) noexcept = default;
};
```

**Rationale:**
- GPU resources are expensive to duplicate
- Prevents accidental copies (would require deep copy of GPU buffers)
- Allows storage in containers via move semantics
- Enables return-by-value optimization (RVO/NRVO)

### 4. Interleaved Vertex Format

Instead of separate buffers (SoA - Structure of Arrays):
```
Buffer 0: [x0, y0, z0, x1, y1, z1, ...]  // positions
Buffer 1: [r0, g0, b0, r1, g1, b1, ...]  // colors
Buffer 2: [u0, v0, u1, v1, ...]          // texcoords
```

We use interleaved format (AoS - Array of Structures):
```
Buffer 0: [x0, y0, z0, r0, g0, b0, u0, v0, nx0, ny0, nz0,
           x1, y1, z1, r1, g1, b1, u1, v1, nx1, ny1, nz1, ...]
```

**Benefits:**
- Better GPU cache locality (all vertex data fetched together)
- Single VBO to manage instead of multiple
- Matches how vertex shaders consume data
- Industry standard for most rendering engines

**Layout:**
```
Offset   Size    Attribute       Location
------   ----    ---------       --------
0        12      Position (vec3) 0
12       12      Color (vec3)    1 (optional)
24       8       TexCoord (vec2) 2 (optional)
32       12      Normal (vec3)   3 (optional)

Total stride: Variable (12, 24, 32, or 44 bytes)
```

## Architecture Deep Dive

### Class Structure

```cpp
class RenderMesh {
private:
    // GPU Resources (owned via unique_ptr)
    std::unique_ptr<IVertexArray> vertexArray_;
    std::unique_ptr<IVertexBuffer> vertexBuffer_;
    std::unique_ptr<IIndexBuffer> indexBuffer_;

    // Renderer reference (for draw calls)
    IRenderer* renderer_;

    // Cached metadata (for validation and rendering)
    size_t vertexCount_;
    size_t indexCount_;
    BufferUsage bufferUsage_;
    PrimitiveType primitiveType_;

    // Vertex layout flags
    bool hasColors_;
    bool hasTexCoords_;
    bool hasNormals_;
};
```

### Construction Flow

```
1. Validate input mesh (check consistency, non-empty)
   └─> validateMesh()

2. Create GPU resources via renderer factory
   ├─> renderer.createVertexArray()
   ├─> renderer.createVertexBuffer()
   └─> renderer.createIndexBuffer()

3. Bind VAO (subsequent bindings captured by VAO state)
   └─> vertexArray_->bind()

4. Interleave vertex data
   └─> interleaveVertexData()
       └─> [pos, col, tex, norm, pos, col, tex, norm, ...]

5. Upload vertex data to VBO
   └─> vertexBuffer_->setData()

6. Configure vertex attributes in VAO
   └─> setupVertexAttributes()
       ├─> addAttribute(location=0, position)
       ├─> addAttribute(location=1, color)    [if present]
       ├─> addAttribute(location=2, texcoord) [if present]
       └─> addAttribute(location=3, normal)   [if present]

7. Upload index data to IBO
   └─> indexBuffer_->setData()

8. Unbind all objects (clean state)
```

### Draw Call Flow

```cpp
void RenderMesh::draw() const {
    // 1. Bind VAO
    //    - Automatically binds associated VBO and IBO
    //    - Restores vertex attribute configuration
    vertexArray_->bind();

    // 2. Issue indexed draw call
    //    - Uses bound IBO for indices
    //    - Renderer translates to API-specific call
    //      * OpenGL: glDrawElements()
    //      * Vulkan: vkCmdDrawIndexed()
    renderer_->drawElements(
        primitiveType_,
        indexCount_,
        indexType_,
        nullptr  // Use bound IBO
    );

    // 3. VAO remains bound (caller may unbind if needed)
}
```

**Why VAO remains bound:**
- Allows batch rendering (bind once, draw many)
- Caller controls state management
- Explicit unbind if state isolation needed

## API Design Decisions

### 1. Constructor Takes Mesh by const Reference

```cpp
RenderMesh(const Mesh& mesh, IRenderer& renderer, ...);
```

**Rationale:**
- Mesh data is copied into GPU buffers anyway
- No need to take ownership of CPU data
- Allows temporary Mesh objects: `RenderMesh(MeshFactory::createCube(), renderer)`
- User retains CPU mesh if needed for physics, collision, etc.

**Alternative considered:** Take `Mesh&&` to transfer ownership
- **Rejected:** Forces user to give up CPU mesh, which may be needed elsewhere

### 2. Renderer Passed by Reference, Not Stored as unique_ptr

```cpp
IRenderer* renderer_;  // Non-owning pointer
```

**Rationale:**
- RenderMesh doesn't own the renderer
- Renderer lifetime managed by application
- Prevents circular ownership issues
- Clear ownership semantics (renderer outlives all meshes)

### 3. draw() is const

```cpp
void draw() const;
```

**Rationale:**
- Drawing doesn't modify CPU-side state
- Allows drawing from const RenderMesh objects
- Enables usage in const methods and with const references
- GPU state changes are external side effects, not object mutation

### 4. Separate update() and updateVertexData()

```cpp
void update(const Mesh& mesh);           // Updates vertices AND indices
void updateVertexData(const Mesh& mesh); // Updates only vertices
```

**Rationale:**
- **Performance:** Updating only vertices is faster (skip index buffer upload)
- **Common use case:** Animated meshes change positions/normals but not topology
- **Flexibility:** User chooses appropriate update granularity

**Example:**
```cpp
// Animation loop (topology unchanged)
for (int frame = 0; frame < 1000; ++frame) {
    updateMeshAnimation(cpuMesh);
    gpuMesh.updateVertexData(cpuMesh);  // Fast path
}
```

### 5. Validation in Constructor

```cpp
RenderMesh::RenderMesh(...) {
    validateMesh(mesh);  // Throws on invalid mesh
    // ... create resources
}
```

**Rationale:**
- **Fail fast:** Catch errors immediately
- **Exception safety:** No partial construction if validation fails
- **Clear error messages:** User knows exactly what's wrong
- **Prevents GPU resource waste:** Don't allocate if data is bad

## Buffer Usage Guidelines

### Static (Default)

**When to use:**
- Geometry that never changes (buildings, terrain, static props)
- Most common case in games

**Performance:**
- GPU may store in fast VRAM
- Optimal read performance
- Updating is expensive (requires reallocation)

```cpp
RenderMesh staticMesh(mesh, renderer, BufferUsage::Static);
```

### Dynamic

**When to use:**
- Geometry that changes occasionally (character models, animated objects)
- Updated a few times per frame or less

**Performance:**
- Balanced between update speed and read speed
- May use staging buffers

```cpp
RenderMesh dynamicMesh(mesh, renderer, BufferUsage::Dynamic);
```

### Stream

**When to use:**
- Geometry that changes every frame (particles, debug lines, UI)
- Frequently regenerated data

**Performance:**
- Optimized for CPU writes
- May use ring buffers or orphaning techniques

```cpp
RenderMesh streamMesh(mesh, renderer, BufferUsage::Stream);
```

## Error Handling Strategy

### Construction Errors

```cpp
try {
    RenderMesh gpuMesh(cpuMesh, renderer);
}
catch (const std::invalid_argument& e) {
    // Mesh validation failed
    // - Empty mesh
    // - Inconsistent attribute sizes
    // - Invalid indices
}
catch (const std::runtime_error& e) {
    // GPU resource creation failed
    // - Out of GPU memory
    // - Driver error
}
```

### Update Errors

```cpp
try {
    gpuMesh.update(newMesh);
}
catch (const std::invalid_argument& e) {
    // Mesh validation failed
}
catch (const std::runtime_error& e) {
    // Vertex layout changed
    // - New mesh has different attributes
    // - Cannot change layout after creation
}
```

**Why layout changes are forbidden:**
- Vertex attribute configuration is stored in VAO
- Changing layout requires recreating VAO
- Prevents subtle bugs from attribute mismatch
- **Solution:** Create new RenderMesh if layout changes

## Performance Characteristics

### Memory Usage

**CPU Memory:**
- Negligible (only stores metadata: counts, flags, pointers)
- ~100 bytes per RenderMesh object

**GPU Memory:**
- Vertex buffer: `vertexCount * stride` bytes
  - Min stride: 12 bytes (position only)
  - Max stride: 44 bytes (position + color + texcoord + normal)
- Index buffer: `indexCount * sizeof(uint32_t)` bytes
  - Optimized to uint16_t or uint8_t if possible
- VAO: Minimal (stores state, not data)

**Example:**
```cpp
// Cube mesh: 24 vertices, 36 indices, full attributes
Mesh cube = MeshFactory::createCube(1.0f);
RenderMesh gpuCube(cube, renderer);

// GPU memory:
// VBO: 24 vertices * 44 bytes = 1,056 bytes
// IBO: 36 indices * 4 bytes = 144 bytes (could optimize to 2 bytes)
// Total: ~1.2 KB
```

### Time Complexity

| Operation               | Complexity           | Notes                              |
|------------------------|----------------------|------------------------------------|
| Construction           | O(n)                 | n = vertex count (interleaving)    |
| draw()                 | O(1)                 | GPU-bound, not CPU-intensive       |
| update()               | O(n)                 | Reinterleave + upload              |
| updateVertexData()     | O(n)                 | Reinterleave + upload (no indices) |
| drawSubset()           | O(1)                 | Same as draw()                     |

### Cache Performance

**Interleaved format improves cache locality:**
- GPU fetches 64-256 byte cache lines
- Interleaved data keeps related attributes together
- Reduces cache misses during vertex processing

**Example:**
```
Without interleaving (3 separate buffers):
Cache line fetch 1: positions[0..4]
Cache line fetch 2: colors[0..4]
Cache line fetch 3: texcoords[0..8]

With interleaving (1 buffer):
Cache line fetch 1: v0.pos, v0.col, v0.tex, v1.pos, v1.col
(More data useful for current vertex processing)
```

## Integration with Rendering Pipeline

### Shader Compatibility

**Vertex shader must match RenderMesh layout:**

```glsl
#version 330 core

// Must match RenderMesh attribute locations
layout(location = 0) in vec3 aPosition;  // Always present
layout(location = 1) in vec3 aColor;     // If hasColors()
layout(location = 2) in vec2 aTexCoord;  // If hasTexCoords()
layout(location = 3) in vec3 aNormal;    // If hasNormals()

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

out vec3 vColor;
out vec2 vTexCoord;
out vec3 vNormal;

void main() {
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
    vNormal = mat3(transpose(inverse(uModelMatrix))) * aNormal;
}
```

### Rendering Loop Pattern

```cpp
void renderScene(
    IRenderer& renderer,
    IShaderProgram& shader,
    const Camera& camera,
    const std::vector<RenderMesh>& meshes
) {
    renderer.clear();

    // Bind shader once for all meshes
    shader.use();

    // Set per-frame uniforms (view, projection)
    shader.setUniform("uViewMatrix", camera.getViewMatrix());
    shader.setUniform("uProjectionMatrix", camera.getProjectionMatrix());

    // Render all meshes
    for (const auto& mesh : meshes) {
        // Set per-mesh uniforms (model matrix)
        glm::mat4 modelMatrix = calculateModelMatrix(mesh);
        shader.setUniform("uModelMatrix", modelMatrix);

        // Draw
        mesh.draw();
    }
}
```

## Extension Points

### Future Enhancements

1. **Instanced Rendering**
```cpp
class RenderMesh {
    void drawInstanced(size_t instanceCount) const;
};
```

2. **Indirect Drawing**
```cpp
class RenderMesh {
    void drawIndirect(IBuffer& indirectBuffer, size_t offset) const;
};
```

3. **Multi-Draw**
```cpp
class RenderMesh {
    static void drawMultiple(
        const std::vector<RenderMesh*>& meshes,
        const std::vector<glm::mat4>& transforms
    );
};
```

4. **Tangent/Bitangent Support**
```cpp
// Add to Mesh class
std::vector<Vec3> tangents_;
std::vector<Vec3> bitangents_;

// Add to RenderMesh
static constexpr unsigned int ATTRIB_TANGENT   = 4;
static constexpr unsigned int ATTRIB_BITANGENT = 5;
```

5. **Compressed Vertex Formats**
```cpp
// Use 16-bit half floats for normals, tangents
// Use 8-bit unsigned bytes for colors (normalized)
enum class VertexCompressionMode {
    None,
    HalfFloatNormals,
    ByteColors
};
```

## Testing Strategy

### Unit Tests

```cpp
TEST(RenderMesh, ConstructionWithValidMesh) {
    MockRenderer renderer;
    Mesh mesh = MeshFactory::createCube(1.0f);

    ASSERT_NO_THROW({
        RenderMesh gpuMesh(mesh, renderer);
        EXPECT_EQ(gpuMesh.getVertexCount(), 24);
        EXPECT_EQ(gpuMesh.getIndexCount(), 36);
    });
}

TEST(RenderMesh, ConstructionWithEmptyMeshThrows) {
    MockRenderer renderer;
    Mesh emptyMesh;

    EXPECT_THROW(
        RenderMesh(emptyMesh, renderer),
        std::invalid_argument
    );
}

TEST(RenderMesh, UpdatePreservesLayout) {
    MockRenderer renderer;
    Mesh mesh1 = MeshFactory::createCube(1.0f);
    RenderMesh gpuMesh(mesh1, renderer);

    Mesh mesh2 = MeshFactory::createCube(2.0f);
    ASSERT_NO_THROW(gpuMesh.update(mesh2));
}

TEST(RenderMesh, UpdateWithDifferentLayoutThrows) {
    MockRenderer renderer;
    Mesh mesh1 = MeshFactory::createCube(1.0f); // Has colors, texcoords, normals
    RenderMesh gpuMesh(mesh1, renderer);

    Mesh mesh2;  // No colors, texcoords, normals
    mesh2.addVertex({0, 0, 0});
    mesh2.addVertex({1, 0, 0});
    mesh2.addVertex({0, 1, 0});
    mesh2.addTriangle(0, 1, 2);

    EXPECT_THROW(
        gpuMesh.update(mesh2),
        std::runtime_error
    );
}
```

## Common Pitfalls and Solutions

### Pitfall 1: Forgetting to Bind Shader

```cpp
// WRONG
renderMesh.draw();  // Shader not bound - undefined behavior!

// CORRECT
shader.use();
renderMesh.draw();
```

### Pitfall 2: Using Static Buffer for Frequent Updates

```cpp
// INEFFICIENT
RenderMesh mesh(cpuMesh, renderer, BufferUsage::Static);
for (int i = 0; i < 1000; ++i) {
    mesh.update(newMesh);  // Slow! Reallocates every frame
}

// EFFICIENT
RenderMesh mesh(cpuMesh, renderer, BufferUsage::Dynamic);
for (int i = 0; i < 1000; ++i) {
    mesh.updateVertexData(newMesh);  // Fast path
}
```

### Pitfall 3: Mixing Coordinate Systems

```cpp
// If your Mesh uses Y-up but renderer expects Z-up
Mesh mesh = MeshFactory::createCube(1.0f);

// Transform before uploading
transformMeshYupToZup(mesh);

RenderMesh gpuMesh(mesh, renderer);
```

### Pitfall 4: Lifetime Issues

```cpp
// WRONG - Renderer destroyed before mesh
{
    IRenderer renderer;
    RenderMesh mesh(cpuMesh, renderer);
}  // renderer destroyed
// mesh.draw();  // Dangling pointer!

// CORRECT - Renderer outlives mesh
IRenderer renderer;
{
    RenderMesh mesh(cpuMesh, renderer);
    mesh.draw();
}  // mesh destroyed first
```

## Conclusion

`RenderMesh` provides a robust, efficient, and safe abstraction for GPU mesh rendering. Its design prioritizes:

1. **Safety:** RAII, move-only semantics, validation
2. **Performance:** Interleaved format, appropriate buffer usage
3. **Usability:** Simple API, clear error messages
4. **Maintainability:** Well-documented, testable, extensible

The class successfully bridges the CPU-GPU boundary while hiding implementation complexity from users.
