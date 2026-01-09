/**
 * @file RenderMeshExample.cpp
 * @brief Comprehensive example demonstrating RenderMesh usage patterns.
 *
 * This file shows various ways to use the RenderMesh class:
 * 1. Basic usage with static geometry
 * 2. Dynamic mesh updates (animated geometry)
 * 3. Using different primitive types
 * 4. Partial mesh rendering (draw subsets)
 * 5. Integration with shader programs
 */

#include "src/RenderMesh.h"
#include "src/RenderAPI/IRenderer.h"
#include "src/RenderAPI/IShaderProgram.h"
#include "Mesh.h"
#include <memory>
#include <vector>
#include <cmath>

using namespace Graphics;

// ============================================================================
// Example 1: Basic Static Mesh Rendering
// ============================================================================

/**
 * @brief Creates and renders a static cube mesh.
 *
 * This is the most common use case: load geometry once, render many times.
 * Use BufferUsage::Static for best GPU performance.
 */
void example_StaticMesh(IRenderer& renderer, IShaderProgram& shader) {
    // Create CPU-side mesh
    Mesh cubeMesh = MeshFactory::createCube(2.0f);

    // Upload to GPU (one-time operation)
    RenderMesh gpuCube(cubeMesh, renderer, BufferUsage::Static);

    // Render loop
    for (int frame = 0; frame < 1000; ++frame) {
        renderer.clear();

        shader.use();
        // Set shader uniforms (MVP matrix, lighting, etc.)

        // Simple draw call - RenderMesh handles all binding
        gpuCube.draw();

        // Present frame...
    }

    // GPU resources automatically cleaned up when gpuCube goes out of scope
}

// ============================================================================
// Example 2: Dynamic Mesh Updates (Animation)
// ============================================================================

/**
 * @brief Demonstrates mesh animation via per-frame vertex updates.
 *
 * Use BufferUsage::Dynamic or Stream when you need to update geometry
 * frequently. This example shows a waving plane effect.
 */
void example_DynamicMesh(IRenderer& renderer, IShaderProgram& shader) {
    // Create initial plane mesh
    Mesh planeMesh = MeshFactory::createPlane(10.0f, 10.0f, 32, 32);

    // Use Dynamic usage for frequent updates
    RenderMesh gpuPlane(planeMesh, renderer, BufferUsage::Dynamic);

    // Animation loop
    for (int frame = 0; frame < 1000; ++frame) {
        const float time = frame * 0.016f; // 60 FPS

        // Modify CPU mesh vertices (create wave effect)
        auto& vertices = planeMesh.getVertices();
        for (size_t i = 0; i < vertices.size(); ++i) {
            auto& v = vertices[i];
            // Wave equation: y = sin(x + time) * cos(z + time)
            v.y = std::sin(v.x + time) * std::cos(v.z + time) * 0.5f;
        }

        // Recompute normals for proper lighting
        planeMesh.computeSmoothNormals();

        // Upload updated geometry to GPU
        gpuPlane.updateVertexData(planeMesh);

        // Render
        renderer.clear();
        shader.use();
        gpuPlane.draw();
    }
}

// ============================================================================
// Example 3: Different Primitive Types
// ============================================================================

/**
 * @brief Shows how to render the same mesh with different primitive types.
 */
void example_PrimitiveTypes(IRenderer& renderer, IShaderProgram& shader) {
    Mesh mesh = MeshFactory::createCube(2.0f);

    // Create mesh with default triangle topology
    RenderMesh renderMesh(mesh, renderer, BufferUsage::Static);

    renderer.clear();
    shader.use();

    // Draw as filled triangles (default)
    renderMesh.draw(PrimitiveType::Triangles);

    // Draw as wireframe (lines)
    renderMesh.draw(PrimitiveType::Lines);

    // Draw as points (vertex positions only)
    renderMesh.draw(PrimitiveType::Points);
}

// ============================================================================
// Example 4: Partial Mesh Rendering (Level of Detail)
// ============================================================================

/**
 * @brief Demonstrates drawing subsets of a mesh for LOD or culling.
 */
void example_PartialRendering(IRenderer& renderer, IShaderProgram& shader) {
    Mesh sphereMesh = MeshFactory::createSphere(1.0f, 64, 32);
    RenderMesh gpuSphere(sphereMesh, renderer, BufferUsage::Static);

    const size_t totalIndices = gpuSphere.getIndexCount();

    shader.use();

    // Full detail (all indices)
    gpuSphere.draw();

    // Half detail (render only first half of mesh)
    gpuSphere.drawSubset(totalIndices / 2, 0);

    // Quarter detail (render first quarter)
    gpuSphere.drawSubset(totalIndices / 4, 0);

    // Render back half only
    gpuSphere.drawSubset(totalIndices / 2, totalIndices / 2);
}

// ============================================================================
// Example 5: Managing Multiple Meshes
// ============================================================================

/**
 * @brief Shows how to manage multiple meshes efficiently.
 *
 * RenderMesh is movable, so it can be stored in containers.
 */
void example_MultipleMeshes(IRenderer& renderer, IShaderProgram& shader) {
    // Store meshes in a vector (uses move semantics)
    std::vector<RenderMesh> meshes;

    // Create various primitives
    meshes.emplace_back(
        MeshFactory::createCube(1.0f),
        renderer,
        BufferUsage::Static
    );

    meshes.emplace_back(
        MeshFactory::createSphere(0.5f, 32, 16),
        renderer,
        BufferUsage::Static
    );

    meshes.emplace_back(
        MeshFactory::createPlane(5.0f, 5.0f, 1, 1),
        renderer,
        BufferUsage::Static
    );

    // Render all meshes
    renderer.clear();
    shader.use();

    for (const auto& mesh : meshes) {
        // Update per-mesh uniforms (model matrix, material properties, etc.)
        mesh.draw();
    }
}

// ============================================================================
// Example 6: Custom Mesh Construction
// ============================================================================

/**
 * @brief Shows how to build a custom mesh and upload it.
 */
void example_CustomMesh(IRenderer& renderer, IShaderProgram& shader) {
    // Build a triangle mesh manually
    Mesh triangleMesh;
    triangleMesh.reserve(3, 3);

    // Define triangle vertices
    triangleMesh.addVertex(
        {-1.0f, -1.0f, 0.0f}, // position
        {1.0f, 0.0f, 0.0f},   // color (red)
        {0.0f, 0.0f},         // texcoord
        {0.0f, 0.0f, 1.0f}    // normal
    );

    triangleMesh.addVertex(
        {1.0f, -1.0f, 0.0f},  // position
        {0.0f, 1.0f, 0.0f},   // color (green)
        {1.0f, 0.0f},         // texcoord
        {0.0f, 0.0f, 1.0f}    // normal
    );

    triangleMesh.addVertex(
        {0.0f, 1.0f, 0.0f},   // position
        {0.0f, 0.0f, 1.0f},   // color (blue)
        {0.5f, 1.0f},         // texcoord
        {0.0f, 0.0f, 1.0f}    // normal
    );

    // Add indices
    triangleMesh.addTriangle(0, 1, 2);

    // Validate before uploading
    if (!triangleMesh.validate()) {
        // Handle error
        return;
    }

    // Upload to GPU
    RenderMesh gpuTriangle(triangleMesh, renderer, BufferUsage::Static);

    // Render
    shader.use();
    gpuTriangle.draw();
}

// ============================================================================
// Example 7: Error Handling and Validation
// ============================================================================

/**
 * @brief Demonstrates proper error handling when creating meshes.
 */
void example_ErrorHandling(IRenderer& renderer) {
    try {
        // Empty mesh - will throw
        Mesh emptyMesh;
        RenderMesh gpuMesh(emptyMesh, renderer); // throws std::invalid_argument
    }
    catch (const std::invalid_argument& e) {
        // Handle validation error
        // Logger::getInstance().log("Mesh validation failed: " + std::string(e.what()));
    }
    catch (const std::runtime_error& e) {
        // Handle GPU resource creation failure
        // Logger::getInstance().log("GPU resource creation failed: " + std::string(e.what()));
    }

    try {
        // Create valid mesh
        Mesh validMesh = MeshFactory::createCube(1.0f);
        RenderMesh gpuMesh(validMesh, renderer);

        // Try to update with incompatible mesh (different attributes)
        Mesh incompatibleMesh;
        incompatibleMesh.addVertex({0, 0, 0}); // No colors, texcoords, normals
        incompatibleMesh.addVertex({1, 0, 0});
        incompatibleMesh.addVertex({0, 1, 0});
        incompatibleMesh.addTriangle(0, 1, 2);

        gpuMesh.update(incompatibleMesh); // throws std::runtime_error
    }
    catch (const std::runtime_error& e) {
        // Handle layout mismatch
        // Logger::getInstance().log("Update failed: " + std::string(e.what()));
    }
}

// ============================================================================
// Example 8: Integration with Shader Vertex Layout
// ============================================================================

/**
 * @brief Shows how RenderMesh vertex attributes map to shader inputs.
 *
 * Shader vertex input layout must match RenderMesh attribute locations:
 *
 * GLSL shader example:
 * @code
 * #version 330 core
 *
 * layout(location = 0) in vec3 aPosition;  // Always present
 * layout(location = 1) in vec3 aColor;     // If mesh.hasColors()
 * layout(location = 2) in vec2 aTexCoord;  // If mesh.hasTexCoords()
 * layout(location = 3) in vec3 aNormal;    // If mesh.hasNormals()
 *
 * void main() {
 *     // Use attributes...
 * }
 * @endcode
 */
void example_ShaderIntegration(IRenderer& renderer, IShaderProgram& shader) {
    // Create mesh with all attributes
    Mesh mesh = MeshFactory::createSphere(1.0f, 32, 16);

    // Shader must have matching vertex input layout
    RenderMesh gpuMesh(mesh, renderer, BufferUsage::Static);

    shader.use();

    // Set uniform values
    // shader.setUniform("uModelMatrix", modelMatrix);
    // shader.setUniform("uViewMatrix", viewMatrix);
    // shader.setUniform("uProjectionMatrix", projMatrix);

    // Draw - attributes automatically bound to correct shader locations
    gpuMesh.draw();
}

// ============================================================================
// Example 9: Performance Considerations
// ============================================================================

/**
 * @brief Demonstrates best practices for performance.
 */
void example_PerformanceOptimization(IRenderer& renderer, IShaderProgram& shader) {
    // ========================================================================
    // DO: Use Static usage for geometry that never changes
    // ========================================================================
    Mesh staticMesh = MeshFactory::createCube(1.0f);
    RenderMesh staticGpuMesh(staticMesh, renderer, BufferUsage::Static);

    // ========================================================================
    // DO: Use Dynamic usage for geometry that changes occasionally
    // ========================================================================
    Mesh dynamicMesh = MeshFactory::createPlane(10.0f, 10.0f, 32, 32);
    RenderMesh dynamicGpuMesh(dynamicMesh, renderer, BufferUsage::Dynamic);

    // ========================================================================
    // DO: Use Stream usage for geometry that changes every frame
    // ========================================================================
    Mesh streamMesh = MeshFactory::createSphere(1.0f, 16, 8);
    RenderMesh streamGpuMesh(streamMesh, renderer, BufferUsage::Stream);

    // ========================================================================
    // DO: Batch state changes and draw calls together
    // ========================================================================
    shader.use();  // Bind shader once

    for (int i = 0; i < 100; ++i) {
        // Update per-instance uniforms (model matrix, etc.)
        // shader.setUniform("uModelMatrix", modelMatrices[i]);

        // Draw (VAO binding is relatively cheap)
        staticGpuMesh.draw();
    }

    // ========================================================================
    // DON'T: Update static buffers frequently
    // ========================================================================
    // staticGpuMesh.update(newMesh); // Inefficient! Use Dynamic instead

    // ========================================================================
    // DO: Use updateVertexData() when only vertices change
    // ========================================================================
    // More efficient than update() if index buffer is unchanged
    // dynamicGpuMesh.updateVertexData(newMesh);
}

// ============================================================================
// Main Function - Run Examples
// ============================================================================

/*
int main() {
    // Assume renderer and shader are initialized
    // IRenderer& renderer = ...;
    // IShaderProgram& shader = ...;

    example_StaticMesh(renderer, shader);
    example_DynamicMesh(renderer, shader);
    example_PrimitiveTypes(renderer, shader);
    example_PartialRendering(renderer, shader);
    example_MultipleMeshes(renderer, shader);
    example_CustomMesh(renderer, shader);
    example_ErrorHandling(renderer);
    example_ShaderIntegration(renderer, shader);
    example_PerformanceOptimization(renderer, shader);

    return 0;
}
*/
