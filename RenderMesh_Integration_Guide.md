# RenderMesh Integration Guide

This guide walks through integrating `RenderMesh` into your existing rendering pipeline.

## Step 1: Add Files to Build System

### CMakeLists.txt

Add the new source files to your project:

```cmake
# Add to your executable or library
add_executable(YourApp
    # ... existing files ...
    src/RenderMesh.h
    src/RenderMesh.cpp
)

# Or if you have a source list variable
set(GRAPHICS_SOURCES
    ${GRAPHICS_SOURCES}
    src/RenderMesh.cpp
)
```

## Step 2: Update Your Application Class

### Before (Manual Buffer Management)

```cpp
class MyApp {
private:
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IShaderProgram> shader_;

    // Manual resource management
    std::unique_ptr<IVertexArray> cubeVAO_;
    std::unique_ptr<IVertexBuffer> cubeVBO_;
    std::unique_ptr<IIndexBuffer> cubeIBO_;

    void initializeCube() {
        // Manually create and configure buffers
        cubeVAO_ = renderer_->createVertexArray();
        cubeVBO_ = renderer_->createVertexBuffer();
        cubeIBO_ = renderer_->createIndexBuffer();

        // Manually interleave vertex data
        std::vector<float> vertexData = {
            // position         // color
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            0, 2, 3
        };

        // Manually upload data
        cubeVAO_->bind();
        cubeVBO_->bind();
        cubeVBO_->setData(vertexData.data(), vertexData.size() * sizeof(float), BufferUsage::Static);

        // Manually configure attributes
        cubeVAO_->addAttribute(VertexAttribute(0, 3, DataType::Float, false, 6 * sizeof(float), (void*)0));
        cubeVAO_->addAttribute(VertexAttribute(1, 3, DataType::Float, false, 6 * sizeof(float), (void*)(3 * sizeof(float))));

        cubeIBO_->bind();
        cubeIBO_->setData(indices.data(), indices.size(), IndexType::UnsignedInt, BufferUsage::Static);

        cubeVAO_->unbind();
    }

    void renderCube() {
        cubeVAO_->bind();
        renderer_->drawElements(PrimitiveType::Triangles, 6, 0, nullptr);
    }
};
```

### After (Using RenderMesh)

```cpp
#include "src/RenderMesh.h"
#include "Mesh.h"

class MyApp {
private:
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IShaderProgram> shader_;

    // Clean, high-level resource
    std::unique_ptr<Graphics::RenderMesh> cubeMesh_;

    void initializeCube() {
        // Simple, declarative mesh creation
        Graphics::Mesh cpuMesh = Graphics::MeshFactory::createCube(1.0f);

        // One-line upload to GPU
        cubeMesh_ = std::make_unique<Graphics::RenderMesh>(
            cpuMesh,
            *renderer_,
            BufferUsage::Static
        );
    }

    void renderCube() {
        // One-line draw call
        cubeMesh_->draw();
    }
};
```

**Benefits:**
- 70% less code
- No manual interleaving
- No manual attribute configuration
- Automatic resource cleanup
- Type-safe and error-checked

## Step 3: Migrate Existing Geometry

### Pattern 1: Replace Manual Vertex Data

**Before:**
```cpp
std::vector<float> triangleVertices = {
    // positions        // colors
    -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f
};
```

**After:**
```cpp
Graphics::Mesh triangleMesh;
triangleMesh.addVertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f});
triangleMesh.addVertex({ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
triangleMesh.addVertex({ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f});
triangleMesh.addTriangle(0, 1, 2);

Graphics::RenderMesh gpuTriangle(triangleMesh, *renderer_);
```

### Pattern 2: Replace Primitive Factories

**Before:**
```cpp
// Your custom sphere generation code
std::vector<float> generateSphere(float radius, int segments) {
    // 100+ lines of code
    // ...
}
```

**After:**
```cpp
Graphics::Mesh sphereMesh = Graphics::MeshFactory::createSphere(radius, segments, segments / 2);
Graphics::RenderMesh gpuSphere(sphereMesh, *renderer_);
```

### Pattern 3: Batch Conversion

```cpp
// Convert all your existing geometry at once
struct MeshRegistry {
    std::unordered_map<std::string, std::unique_ptr<Graphics::RenderMesh>> meshes_;
    IRenderer& renderer_;

    MeshRegistry(IRenderer& renderer) : renderer_(renderer) {}

    void loadStandardPrimitives() {
        meshes_["cube"] = std::make_unique<Graphics::RenderMesh>(
            Graphics::MeshFactory::createCube(1.0f), renderer_
        );

        meshes_["sphere"] = std::make_unique<Graphics::RenderMesh>(
            Graphics::MeshFactory::createSphere(1.0f, 32, 16), renderer_
        );

        meshes_["plane"] = std::make_unique<Graphics::RenderMesh>(
            Graphics::MeshFactory::createPlane(10.0f, 10.0f, 1, 1), renderer_
        );
    }

    Graphics::RenderMesh* get(const std::string& name) {
        auto it = meshes_.find(name);
        return it != meshes_.end() ? it->second.get() : nullptr;
    }
};

// Usage
MeshRegistry meshRegistry(*renderer_);
meshRegistry.loadStandardPrimitives();

// In render loop
if (auto* cube = meshRegistry.get("cube")) {
    cube->draw();
}
```

## Step 4: Update Render Loop

### Basic Rendering

```cpp
void MyApp::render() {
    renderer_->clear();

    // Bind shader
    shader_->use();

    // Set uniforms
    shader_->setUniform("uViewMatrix", camera_->getViewMatrix());
    shader_->setUniform("uProjectionMatrix", camera_->getProjectionMatrix());

    // Draw each mesh
    for (const auto& [name, mesh] : meshes_) {
        glm::mat4 modelMatrix = calculateModelMatrix(name);
        shader_->setUniform("uModelMatrix", modelMatrix);

        mesh->draw();
    }
}
```

### Instanced Rendering (Multiple Objects, Same Mesh)

```cpp
void MyApp::renderForest() {
    shader_->use();

    // Draw 1000 trees using same mesh
    Graphics::RenderMesh* treeMesh = meshRegistry_.get("tree");

    for (int i = 0; i < 1000; ++i) {
        glm::mat4 modelMatrix = treePositions_[i];
        shader_->setUniform("uModelMatrix", modelMatrix);

        treeMesh->draw();  // VAO binding is relatively cheap
    }
}
```

## Step 5: Add Dynamic Mesh Support (Optional)

If you have animated geometry:

```cpp
class AnimatedWater {
private:
    Graphics::Mesh cpuMesh_;
    std::unique_ptr<Graphics::RenderMesh> gpuMesh_;
    float time_ = 0.0f;

public:
    AnimatedWater(IRenderer& renderer) {
        cpuMesh_ = Graphics::MeshFactory::createPlane(50.0f, 50.0f, 64, 64);

        // Use Dynamic for frequent updates
        gpuMesh_ = std::make_unique<Graphics::RenderMesh>(
            cpuMesh_,
            renderer,
            BufferUsage::Dynamic
        );
    }

    void update(float deltaTime) {
        time_ += deltaTime;

        // Animate vertices
        auto& vertices = cpuMesh_.getVertices();
        for (auto& v : vertices) {
            v.y = std::sin(v.x + time_) * std::cos(v.z + time_) * 0.5f;
        }

        // Recompute normals for lighting
        cpuMesh_.computeSmoothNormals();

        // Upload to GPU (efficient for Dynamic buffers)
        gpuMesh_->updateVertexData(cpuMesh_);
    }

    void draw() {
        gpuMesh_->draw();
    }
};
```

## Step 6: Update Shaders (If Needed)

Ensure your vertex shaders use the correct attribute locations:

```glsl
#version 330 core

// RenderMesh attribute locations
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aNormal;

// Rest of shader...
```

If you were using different locations before:

```glsl
// OLD (don't use)
layout(location = 0) in vec3 position;     // OK
layout(location = 5) in vec3 color;        // WRONG
layout(location = 3) in vec2 texCoord;     // WRONG
layout(location = 7) in vec3 normal;       // WRONG

// NEW (use this)
layout(location = 0) in vec3 aPosition;    // OK
layout(location = 1) in vec3 aColor;       // OK
layout(location = 2) in vec2 aTexCoord;    // OK
layout(location = 3) in vec3 aNormal;      // OK
```

## Step 7: Handle Model Loading (Optional)

If you load models from files (OBJ, FBX, GLTF), create a converter:

```cpp
Graphics::Mesh loadModelFromFile(const std::string& filepath) {
    Graphics::Mesh mesh;

    // Use your existing model loader (Assimp, TinyOBJ, etc.)
    // Example with hypothetical loader:
    ModelData data = ModelLoader::load(filepath);

    // Convert to Mesh format
    mesh.reserve(data.vertexCount, data.indexCount);

    for (size_t i = 0; i < data.vertexCount; ++i) {
        mesh.addVertex(
            {data.positions[i].x, data.positions[i].y, data.positions[i].z},
            {data.colors[i].r, data.colors[i].g, data.colors[i].b},
            {data.texCoords[i].u, data.texCoords[i].v},
            {data.normals[i].x, data.normals[i].y, data.normals[i].z}
        );
    }

    for (size_t i = 0; i < data.indexCount; ++i) {
        mesh.addIndex(data.indices[i]);
    }

    return mesh;
}

// Usage
Graphics::Mesh bunnyMesh = loadModelFromFile("models/bunny.obj");
Graphics::RenderMesh gpuBunny(bunnyMesh, *renderer_);
```

## Step 8: Error Handling

Add proper error handling for mesh operations:

```cpp
std::unique_ptr<Graphics::RenderMesh> loadMeshSafe(
    const Graphics::Mesh& mesh,
    IRenderer& renderer
) {
    try {
        return std::make_unique<Graphics::RenderMesh>(mesh, renderer);
    }
    catch (const std::invalid_argument& e) {
        Logger::getInstance().log(
            "Mesh validation failed: " + std::string(e.what())
        );
        return nullptr;
    }
    catch (const std::runtime_error& e) {
        Logger::getInstance().log(
            "GPU resource creation failed: " + std::string(e.what())
        );
        return nullptr;
    }
}
```

## Step 9: Debugging Tips

### Visualize Mesh Bounds

```cpp
void drawMeshBounds(const Graphics::Mesh& mesh, Graphics::RenderMesh& debugLines) {
    // Find bounding box
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& v : mesh.getVertices()) {
        min = glm::min(min, glm::vec3(v.x, v.y, v.z));
        max = glm::max(max, glm::vec3(v.x, v.y, v.z));
    }

    // Create debug box mesh
    Graphics::Mesh boxMesh;
    // ... add box vertices and indices ...

    Graphics::RenderMesh debugBox(boxMesh, *renderer_);
    debugBox.draw(PrimitiveType::Lines);
}
```

### Validate All Meshes on Load

```cpp
void MyApp::loadMeshes() {
    auto meshes = {
        "cube", "sphere", "plane", "cylinder"
    };

    for (const auto& name : meshes) {
        Graphics::Mesh mesh = createMeshByName(name);

        if (!mesh.validate()) {
            Logger::getInstance().log("Invalid mesh: " + name);
            continue;
        }

        meshes_[name] = std::make_unique<Graphics::RenderMesh>(
            mesh, *renderer_
        );

        Logger::getInstance().log(
            "Loaded " + name + ": " +
            std::to_string(mesh.getVertexCount()) + " vertices, " +
            std::to_string(mesh.getIndexCount()) + " indices"
        );
    }
}
```

## Step 10: Performance Profiling

Add timers to measure impact:

```cpp
#include <chrono>

void MyApp::profileMeshUpload() {
    Graphics::Mesh mesh = Graphics::MeshFactory::createSphere(1.0f, 128, 64);

    auto start = std::chrono::high_resolution_clock::now();

    Graphics::RenderMesh gpuMesh(mesh, *renderer_, BufferUsage::Static);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    Logger::getInstance().log(
        "Mesh upload took: " + std::to_string(duration.count()) + " microseconds"
    );
}
```

## Complete Example: Before and After

### Before (200+ lines)

```cpp
class MyApp {
    // Complex manual buffer management
    // Error-prone interleaving
    // No validation
    // Manual cleanup required
};
```

### After (50 lines)

```cpp
class MyApp {
private:
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IShaderProgram> shader_;
    std::unordered_map<std::string, std::unique_ptr<Graphics::RenderMesh>> meshes_;

public:
    void initialize() {
        // Create meshes
        meshes_["cube"] = std::make_unique<Graphics::RenderMesh>(
            Graphics::MeshFactory::createCube(1.0f), *renderer_
        );

        meshes_["sphere"] = std::make_unique<Graphics::RenderMesh>(
            Graphics::MeshFactory::createSphere(1.0f, 32, 16), *renderer_
        );
    }

    void render() {
        renderer_->clear();
        shader_->use();

        shader_->setUniform("uViewMatrix", camera_->getViewMatrix());
        shader_->setUniform("uProjectionMatrix", camera_->getProjectionMatrix());

        for (const auto& [name, mesh] : meshes_) {
            glm::mat4 modelMatrix = calculateModelMatrix(name);
            shader_->setUniform("uModelMatrix", modelMatrix);
            mesh->draw();
        }
    }

    // Automatic cleanup via unique_ptr
};
```

## Migration Checklist

- [ ] Add RenderMesh.h and RenderMesh.cpp to build system
- [ ] Include RenderMesh.h in relevant source files
- [ ] Replace manual buffer creation with RenderMesh
- [ ] Update shader attribute locations (0, 1, 2, 3)
- [ ] Test with simple primitives (cube, sphere)
- [ ] Migrate existing geometry data
- [ ] Add error handling for mesh operations
- [ ] Profile performance impact
- [ ] Update documentation and comments
- [ ] Test dynamic mesh updates if needed
- [ ] Verify all rendering paths work correctly

## Troubleshooting

### Issue: Mesh Doesn't Appear

**Check:**
1. Shader is bound before draw()
2. Camera matrices are correct
3. Mesh is not culled (check winding order)
4. Depth testing is configured correctly

### Issue: Incorrect Colors

**Check:**
1. Mesh has color attributes (mesh.hasColors())
2. Shader reads from correct location (layout location = 1)
3. Color values are in 0-1 range

### Issue: Performance Degradation

**Check:**
1. Using correct BufferUsage (Static for static geometry)
2. Not updating buffers unnecessarily
3. Batching draw calls properly

### Issue: Validation Errors

**Check:**
1. All attribute arrays have same size
2. Indices are within valid range
3. Index count is multiple of 3 (for triangles)

## Next Steps

1. **Integrate with Scene Graph:** Store RenderMesh in scene nodes
2. **Add Material System:** Associate materials with meshes
3. **Implement LOD:** Use drawSubset() for level-of-detail
4. **Optimize Instancing:** Batch identical meshes
5. **Add Mesh Compression:** Reduce GPU memory usage

## Support

If you encounter issues:
1. Check RenderMesh_Design.md for architectural details
2. Review RenderMeshExample.cpp for usage patterns
3. Enable debug logging to trace GPU operations
4. Verify mesh data with mesh.validate()
