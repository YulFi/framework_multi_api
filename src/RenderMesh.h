#pragma once

#include "../Mesh.h"
#include "RenderAPI/IVertexArray.h"
#include "RenderAPI/IVertexBuffer.h"
#include "RenderAPI/IIndexBuffer.h"
#include "RenderAPI/IRenderer.h"
#include "RenderAPI/IPrimitiveType.h"
#include <memory>
#include <cstddef>
#include <stdexcept>

namespace Graphics {

/**
 * @brief GPU-side mesh representation that manages rendering resources.
 *
 * RenderMesh bridges the gap between CPU-side Mesh data and GPU rendering.
 * It handles:
 * - Vertex data interleaving (position, color, texcoord, normal)
 * - GPU buffer creation and management (VAO, VBO, IBO)
 * - Binding and draw call encapsulation
 * - Resource lifetime management via RAII
 *
 * Design decisions:
 * - Non-copyable (GPU resources cannot be duplicated easily)
 * - Movable (allows storage in containers and transfer of ownership)
 * - Uses unique_ptr for GPU resources (automatic cleanup, clear ownership)
 * - Interleaved vertex format for optimal GPU cache performance
 * - Immutable after construction (call update() to change data)
 *
 * Vertex layout (interleaved):
 *   [position.xyz][color.rgb][texcoord.uv][normal.xyz]
 *   - Position: 3 floats (12 bytes) - location 0
 *   - Color:    3 floats (12 bytes) - location 1
 *   - TexCoord: 2 floats (8 bytes)  - location 2
 *   - Normal:   3 floats (12 bytes) - location 3
 *   Total stride: 44 bytes per vertex
 *
 * Usage example:
 * @code
 *   Mesh cpuMesh = MeshFactory::createCube(2.0f);
 *   RenderMesh gpuMesh(cpuMesh, renderer, BufferUsage::Static);
 *
 *   // In render loop:
 *   gpuMesh.draw();  // Simple!
 * @endcode
 */
class RenderMesh {
public:
    // ========================================================================
    // Construction and Rule of Five
    // ========================================================================

    /**
     * @brief Constructs a GPU mesh from CPU mesh data.
     * @param mesh CPU-side mesh containing geometry data
     * @param renderer Renderer interface for creating GPU resources
     * @param usage Buffer usage hint (Static, Dynamic, Stream)
     * @param primitiveType Primitive topology (default: Triangles)
     * @throws std::invalid_argument if mesh is empty or invalid
     * @throws std::runtime_error if GPU resource creation fails
     */
    RenderMesh(
        const Mesh& mesh,
        IRenderer& renderer,
        BufferUsage usage = BufferUsage::Static,
        PrimitiveType primitiveType = PrimitiveType::Triangles
    );

    /**
     * @brief Destructor - GPU resources automatically released via unique_ptr.
     */
    ~RenderMesh() = default;

    // Non-copyable (GPU resources shouldn't be implicitly duplicated)
    RenderMesh(const RenderMesh&) = delete;
    RenderMesh& operator=(const RenderMesh&) = delete;

    // Movable (allows storage in containers and transfer of ownership)
    RenderMesh(RenderMesh&&) noexcept = default;
    RenderMesh& operator=(RenderMesh&&) noexcept = default;

    // ========================================================================
    // Rendering Interface
    // ========================================================================

    /**
     * @brief Renders the mesh using indexed drawing.
     *
     * This method:
     * 1. Binds the vertex array (which binds VBO and IBO internally)
     * 2. Issues indexed draw call with stored primitive type
     * 3. VAO remains bound after call (caller can unbind if needed)
     *
     * @note The shader program must be bound before calling draw()
     * @note This is a const method - rendering doesn't modify CPU state
     */
    void draw() const;

    /**
     * @brief Renders the mesh with explicit primitive type override.
     * @param primitiveType The primitive topology to use for this draw call
     */
    void draw(PrimitiveType primitiveType) const;

    /**
     * @brief Renders a subset of the mesh.
     * @param indexCount Number of indices to draw
     * @param indexOffset Starting index offset (in number of indices, not bytes)
     */
    void drawSubset(size_t indexCount, size_t indexOffset = 0) const;

    // ========================================================================
    // Update Interface
    // ========================================================================

    /**
     * @brief Updates GPU buffers with new mesh data.
     *
     * This method:
     * - Reinterleaves vertex data from the new mesh
     * - Updates vertex buffer (may resize if needed)
     * - Updates index buffer (may resize if needed)
     * - Preserves buffer usage hint from construction
     *
     * @param mesh New mesh data to upload
     * @throws std::invalid_argument if mesh is empty or invalid
     * @throws std::runtime_error if vertex layout changed (different attributes)
     *
     * @note For Static buffers, this is inefficient - prefer Dynamic/Stream usage
     * @note Consider using updateVertexData() if only vertices change
     */
    void update(const Mesh& mesh);

    /**
     * @brief Updates only vertex data (positions, colors, texcoords, normals).
     *
     * More efficient than update() if index buffer remains unchanged.
     * @param mesh Mesh containing new vertex data (must have same vertex count)
     * @throws std::invalid_argument if vertex count changed
     */
    void updateVertexData(const Mesh& mesh);

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Returns the number of indices in the mesh.
     */
    [[nodiscard]] size_t getIndexCount() const noexcept { return indexCount_; }

    /**
     * @brief Returns the number of vertices in the mesh.
     */
    [[nodiscard]] size_t getVertexCount() const noexcept { return vertexCount_; }

    /**
     * @brief Returns the primitive type used for rendering.
     */
    [[nodiscard]] PrimitiveType getPrimitiveType() const noexcept { return primitiveType_; }

    /**
     * @brief Returns the vertex layout flags.
     */
    [[nodiscard]] bool hasColors() const noexcept { return hasColors_; }
    [[nodiscard]] bool hasTexCoords() const noexcept { return hasTexCoords_; }
    [[nodiscard]] bool hasNormals() const noexcept { return hasNormals_; }

    /**
     * @brief Calculates the stride of the interleaved vertex format.
     * @return Stride in bytes
     */
    [[nodiscard]] size_t getVertexStride() const noexcept;

    // ========================================================================
    // Advanced Binding Control
    // ========================================================================

    /**
     * @brief Manually binds the vertex array.
     *
     * Typically not needed - draw() handles binding automatically.
     * Useful for advanced rendering techniques or debugging.
     */
    void bind() const { vertexArray_->bind(); }

    /**
     * @brief Manually unbinds the vertex array.
     */
    void unbind() const { vertexArray_->unbind(); }

private:
    // ========================================================================
    // Internal Helper Methods
    // ========================================================================

    /**
     * @brief Interleaves vertex data into a contiguous buffer.
     *
     * Creates packed vertex data in the format:
     * [v0.pos, v0.col, v0.tex, v0.norm, v1.pos, v1.col, ...]
     *
     * @param mesh Source mesh with separate attribute arrays
     * @param outBuffer Output buffer (will be resized appropriately)
     *
     * Invariants:
     * - Positions are always present
     * - Other attributes are optional (based on mesh.has*() methods)
     * - All present attributes must have same count as positions
     */
    void interleaveVertexData(const Mesh& mesh, std::vector<float>& outBuffer) const;

    /**
     * @brief Sets up vertex attribute pointers in the VAO.
     *
     * Configures the vertex array to interpret interleaved data:
     * - Location 0: position (vec3)
     * - Location 1: color    (vec3) - if present
     * - Location 2: texcoord (vec2) - if present
     * - Location 3: normal   (vec3) - if present
     *
     * @param stride Stride in bytes between consecutive vertices
     */
    void setupVertexAttributes(size_t stride);

    /**
     * @brief Validates that mesh data is consistent and renderable.
     * @throws std::invalid_argument if validation fails
     */
    void validateMesh(const Mesh& mesh) const;

    // ========================================================================
    // Member Data
    // ========================================================================

    // GPU Resources (managed via unique_ptr for automatic cleanup)
    std::unique_ptr<IVertexArray> vertexArray_;   // VAO - encapsulates vertex state
    std::unique_ptr<IVertexBuffer> vertexBuffer_; // VBO - interleaved vertex data
    std::unique_ptr<IIndexBuffer> indexBuffer_;   // IBO - index data

    // Renderer reference (needed for draw calls)
    IRenderer* renderer_;

    // Mesh metadata (cached for efficient rendering and validation)
    size_t vertexCount_;           // Number of vertices
    size_t indexCount_;            // Number of indices
    BufferUsage bufferUsage_;      // Usage hint for GPU buffers
    PrimitiveType primitiveType_;  // Rendering topology

    // Vertex layout flags (determine what attributes are present)
    bool hasColors_;    // Whether mesh has per-vertex colors
    bool hasTexCoords_; // Whether mesh has texture coordinates
    bool hasNormals_;   // Whether mesh has normals

    // Constants for vertex attribute locations (match shader expectations)
    static constexpr unsigned int ATTRIB_POSITION  = 0;
    static constexpr unsigned int ATTRIB_COLOR     = 1;
    static constexpr unsigned int ATTRIB_TEXCOORD  = 2;
    static constexpr unsigned int ATTRIB_NORMAL    = 3;
};

} // namespace Graphics
