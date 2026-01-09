/**
 * @file ExampleUsage.cpp
 * @brief Demonstrates proper usage of the Mesh and Renderable classes
 */

#include "Mesh.h"
#include "Renderable.h"
#include <memory>
#include <vector>
#include <iostream>

using namespace Graphics;

// ============================================================================
// Example 1: Creating a simple mesh manually
// ============================================================================

void example1_ManualMeshCreation() {
    std::cout << "=== Example 1: Manual Mesh Creation ===" << std::endl;

    // Create an empty mesh
    Mesh triangleMesh;

    // Add vertices with positions, colors, and texture coordinates
    triangleMesh.addVertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f});
    triangleMesh.addVertex({ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
    triangleMesh.addVertex({ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f});

    // Add indices for a single triangle
    triangleMesh.addTriangle(0, 1, 2);

    // Compute normals automatically
    triangleMesh.computeFlatNormals();

    // Validate the mesh
    if (triangleMesh.validate()) {
        std::cout << "Triangle mesh is valid!" << std::endl;
        std::cout << "Vertices: " << triangleMesh.getVertexCount() << std::endl;
        std::cout << "Indices: " << triangleMesh.getIndexCount() << std::endl;
        std::cout << "Triangles: " << triangleMesh.getTriangleCount() << std::endl;
    }

    std::cout << std::endl;
}

// ============================================================================
// Example 2: Using mesh factory functions
// ============================================================================

void example2_MeshFactory() {
    std::cout << "=== Example 2: Mesh Factory Functions ===" << std::endl;

    // Create various primitive meshes
    Mesh cube = MeshFactory::createCube(2.0f);
    Mesh sphere = MeshFactory::createSphere(1.0f, 32, 16);
    Mesh plane = MeshFactory::createPlane(10.0f, 10.0f, 10, 10);

    std::cout << "Cube - Vertices: " << cube.getVertexCount()
              << ", Triangles: " << cube.getTriangleCount() << std::endl;

    std::cout << "Sphere - Vertices: " << sphere.getVertexCount()
              << ", Triangles: " << sphere.getTriangleCount() << std::endl;

    std::cout << "Plane - Vertices: " << plane.getVertexCount()
              << ", Triangles: " << plane.getTriangleCount() << std::endl;

    // Meshes are value types - copying is safe
    Mesh cubeCopy = cube; // Deep copy
    std::cout << "Copied cube has " << cubeCopy.getVertexCount() << " vertices" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 3: Creating a Renderable object
// ============================================================================

void example3_CreateRenderable() {
    std::cout << "=== Example 3: Creating Renderables ===" << std::endl;

    // Create a mesh (or load from file in real application)
    auto meshPtr = std::make_shared<Mesh>(MeshFactory::createCube(1.0f));

    // Create a renderable with just a mesh
    Renderable cubeRenderable(meshPtr);

    // In a real application, you'd create shader and texture resources
    // For demonstration, assume we have these:
    // auto shader = std::make_shared<Shader>("shaders/basic.vert", "shaders/basic.frag");
    // auto texture = std::make_shared<Texture>("textures/wood.png");

    // Set resources
    // cubeRenderable.setShader(shader);
    // cubeRenderable.setTexture(texture);

    // Configure rendering properties
    cubeRenderable.setEnabled(true);
    cubeRenderable.setCastsShadows(true);
    cubeRenderable.setReceivesShadows(true);

    // Upload mesh data to GPU
    // cubeRenderable.uploadToGPU(); // Would actually upload in real implementation

    std::cout << "Renderable created and configured" << std::endl;
    std::cout << "Ready to render: " << (cubeRenderable.isReadyToRender() ? "Yes" : "No") << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 4: Sharing meshes across multiple renderables (instancing pattern)
// ============================================================================

void example4_MeshSharing() {
    std::cout << "=== Example 4: Mesh Sharing (Instancing) ===" << std::endl;

    // Create one mesh that will be shared
    auto sharedMesh = std::make_shared<Mesh>(MeshFactory::createSphere(0.5f, 16, 8));

    std::cout << "Created shared sphere mesh with "
              << sharedMesh->getVertexCount() << " vertices" << std::endl;

    // Create multiple renderables using the same mesh
    // Each renderable can have different textures, shaders, or positions
    std::vector<Renderable> spheres;

    for (int i = 0; i < 5; ++i) {
        Renderable sphere(sharedMesh);

        // Each could have different materials/textures
        // sphere.setTexture(differentTextures[i]);

        spheres.push_back(std::move(sphere)); // Move semantics - efficient
    }

    std::cout << "Created " << spheres.size() << " renderables sharing one mesh" << std::endl;
    std::cout << "Memory efficiency: Only one copy of geometry in CPU memory" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 5: Modifying mesh data and re-uploading
// ============================================================================

void example5_MeshModification() {
    std::cout << "=== Example 5: Mesh Modification ===" << std::endl;

    auto mesh = std::make_shared<Mesh>(MeshFactory::createCube(1.0f));
    Renderable renderable(mesh);

    // Simulate GPU upload
    // renderable.uploadToGPU();
    std::cout << "Initial upload complete" << std::endl;

    // Modify the mesh (e.g., vertex animation, deformation)
    auto& vertices = mesh->getVertices();
    for (auto& vertex : vertices) {
        vertex.y += 0.5f; // Move all vertices up
    }

    std::cout << "Mesh data modified on CPU" << std::endl;

    // Mark GPU data as stale
    renderable.invalidateGPUData();

    // Re-upload to GPU
    // renderable.uploadToGPU();
    std::cout << "Re-uploaded modified data to GPU" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 6: Complex material setup with multiple textures
// ============================================================================

void example6_ComplexMaterial() {
    std::cout << "=== Example 6: Complex Material Setup ===" << std::endl;

    auto mesh = std::make_shared<Mesh>(MeshFactory::createSphere(1.0f));
    Renderable renderable(mesh);

    // In real application, load these from disk
    // auto diffuseMap = std::make_shared<Texture>("textures/diffuse.png");
    // auto normalMap = std::make_shared<Texture>("textures/normal.png");
    // auto specularMap = std::make_shared<Texture>("textures/specular.png");
    // auto roughnessMap = std::make_shared<Texture>("textures/roughness.png");

    // Set multiple textures for PBR rendering
    // renderable.setTexture(diffuseMap); // Primary texture
    // renderable.setTexture("normalMap", normalMap);
    // renderable.setTexture("specularMap", specularMap);
    // renderable.setTexture("roughnessMap", roughnessMap);

    // Set custom shader parameters
    // renderable.setParameter("metallic", 0.8f);
    // renderable.setParameter("roughness", 0.3f);
    // renderable.setParameter("ambientOcclusion", 1.0f);

    std::cout << "Complex PBR material configured with multiple texture maps" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 7: Proper resource management and cleanup
// ============================================================================

void example7_ResourceManagement() {
    std::cout << "=== Example 7: Resource Management ===" << std::endl;

    {
        // Resources are managed with smart pointers
        auto mesh = std::make_shared<Mesh>(MeshFactory::createCube());

        {
            // Create renderable in inner scope
            Renderable renderable(mesh);
            // renderable.uploadToGPU();

            std::cout << "Renderable created and GPU resources allocated" << std::endl;

            // When renderable goes out of scope, GPU resources are freed
        } // <-- Renderable destructor called here, GPU buffers released

        std::cout << "Renderable destroyed, GPU resources freed" << std::endl;
        std::cout << "Mesh still exists (reference count = 1)" << std::endl;

    } // <-- Mesh destroyed here (reference count = 0)

    std::cout << "Mesh destroyed, CPU memory freed" << std::endl;
    std::cout << "All cleanup automatic via RAII!" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// Example 8: Batch rendering pattern
// ============================================================================

void example8_BatchRendering() {
    std::cout << "=== Example 8: Batch Rendering Pattern ===" << std::endl;

    // Create multiple renderables
    std::vector<Renderable> renderables;

    auto cubeMesh = std::make_shared<Mesh>(MeshFactory::createCube());
    auto sphereMesh = std::make_shared<Mesh>(MeshFactory::createSphere());

    // Create different objects
    renderables.emplace_back(cubeMesh);
    renderables.emplace_back(sphereMesh);
    renderables.emplace_back(cubeMesh); // Reuse mesh

    std::cout << "Created " << renderables.size() << " renderables" << std::endl;

    // Simulated render loop
    std::cout << "\nSimulated rendering:" << std::endl;
    for (size_t i = 0; i < renderables.size(); ++i) {
        auto& renderable = renderables[i];

        if (!renderable.isEnabled()) {
            continue; // Skip disabled objects
        }

        if (!renderable.isReadyToRender()) {
            std::cout << "Renderable " << i << " not ready, skipping" << std::endl;
            continue;
        }

        // In real implementation:
        // renderable.bind();           // Bind textures and state
        // renderable.render();         // Issue draw call
        // renderable.unbind();         // Clean up state

        std::cout << "Rendered object " << i << std::endl;
    }

    std::cout << std::endl;
}

// ============================================================================
// Example 9: Mesh validation and error handling
// ============================================================================

void example9_ErrorHandling() {
    std::cout << "=== Example 9: Validation and Error Handling ===" << std::endl;

    // Create an invalid mesh (missing indices)
    Mesh invalidMesh;
    invalidMesh.addVertex({0, 0, 0});
    // No indices added!

    if (!invalidMesh.isValid()) {
        std::cout << "Mesh validation failed: Missing indices" << std::endl;
    }

    // Create mesh with mismatched attributes
    Mesh inconsistentMesh;
    inconsistentMesh.addVertex({0, 0, 0});
    inconsistentMesh.addVertex({1, 0, 0});
    inconsistentMesh.addVertex({0, 1, 0});
    inconsistentMesh.addTriangle(0, 1, 2);

    // Add color for only one vertex (inconsistent!)
    inconsistentMesh.getColors().push_back({1, 0, 0});

    if (!inconsistentMesh.validate()) {
        std::cout << "Mesh validation failed: Inconsistent attribute sizes" << std::endl;
        std::cout << "Vertices: " << inconsistentMesh.getVertexCount()
                  << ", Colors: " << inconsistentMesh.getColors().size() << std::endl;
    }

    // Create a valid mesh
    Mesh validMesh = MeshFactory::createCube();
    if (validMesh.validate()) {
        std::cout << "Cube mesh is valid and consistent!" << std::endl;
    }

    std::cout << std::endl;
}

// ============================================================================
// Main function - run all examples
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mesh and Renderable Usage Examples" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try {
        example1_ManualMeshCreation();
        example2_MeshFactory();
        example3_CreateRenderable();
        example4_MeshSharing();
        example5_MeshModification();
        example6_ComplexMaterial();
        example7_ResourceManagement();
        example8_BatchRendering();
        example9_ErrorHandling();

        std::cout << "========================================" << std::endl;
        std::cout << "  All examples completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/**
 * COMPILATION AND USAGE NOTES:
 *
 * To compile this example (assuming you have the implementation files):
 *
 *   g++ -std=c++17 -o example ExampleUsage.cpp Mesh.cpp -I.
 *
 * Or with CMake:
 *
 *   add_executable(example ExampleUsage.cpp Mesh.cpp)
 *   target_compile_features(example PRIVATE cxx_std_17)
 *
 * Key Takeaways:
 *
 * 1. Mesh is a value type - copy it freely, it's just data
 * 2. Renderable is move-only - represents GPU resources
 * 3. Use shared_ptr for resources that are expensive and reusable
 * 4. Always validate meshes before uploading to GPU
 * 5. RAII ensures automatic cleanup of all resources
 * 6. Separation of mesh and renderable allows for flexible instancing
 */
