#pragma once

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>
#include <memory>

namespace Graphics {

/**
 * @brief Represents pure geometry data for rendering.
 *
 * A Mesh is a lightweight, value-semantic container for vertex data.
 * It contains CPU-side geometry information and can be shared across
 * multiple Renderables. This class owns its data and is fully copyable.
 *
 * Design rationale:
 * - Value semantics allow easy copying and storage in containers
 * - Separate types (Vec3, Vec2) provide type safety
 * - Data is kept contiguous for cache efficiency
 * - No GPU-specific code - pure data representation
 */
class Mesh {
public:
    // Type aliases for clarity and flexibility
    typedef glm::vec3 Vec3;
    typedef glm::vec2 Vec2;

    //struct Vec3 {
    //    float x, y, z;

    //    Vec3() = default;
    //    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    //    // Useful for array-style access
    //    float& operator[](size_t i) { return (&x)[i]; }
    //    const float& operator[](size_t i) const { return (&x)[i]; }
    //};

    //struct Vec2 {
    //    float u, v;

    //    Vec2() = default;
    //    Vec2(float u, float v) : u(u), v(v) {}

    //    float& operator[](size_t i) { return (&u)[i]; }
    //    const float& operator[](size_t i) const { return (&u)[i]; }
    //};

    using Index = uint32_t; // uint32_t is standard for most use cases

    // ============================================================================
    // Constructors and Rule of Zero
    // ============================================================================

    /**
     * @brief Default constructor creates an empty mesh.
     */
    Mesh() = default;

    /**
     * @brief Constructs a mesh with preallocated capacity.
     * @param vertexCount Expected number of vertices
     * @param indexCount Expected number of indices
     */
    Mesh(size_t vertexCount, size_t indexCount);

    // Rule of Zero: Let compiler generate copy/move operations
    // std::vector handles everything correctly
    ~Mesh() = default;
    Mesh(const Mesh&) = default;
    Mesh& operator=(const Mesh&) = default;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    // ============================================================================
    // Data Access - const correctness enforced
    // ============================================================================

    [[nodiscard]] const std::vector<Vec3>& getVertices() const noexcept { return vertices_; }
    [[nodiscard]] const std::vector<Index>& getIndices() const noexcept { return indices_; }
    [[nodiscard]] const std::vector<Vec3>& getColors() const noexcept { return colors_; }
    [[nodiscard]] const std::vector<Vec2>& getTexCoords() const noexcept { return texCoords_; }
    [[nodiscard]] const std::vector<Vec3>& getNormals() const noexcept { return normals_; }

    // Non-const access for building/modifying the mesh
    [[nodiscard]] std::vector<Vec3>& getVertices() noexcept { return vertices_; }
    [[nodiscard]] std::vector<Index>& getIndices() noexcept { return indices_; }
    [[nodiscard]] std::vector<Vec3>& getColors() noexcept { return colors_; }
    [[nodiscard]] std::vector<Vec2>& getTexCoords() noexcept { return texCoords_; }
    [[nodiscard]] std::vector<Vec3>& getNormals() noexcept { return normals_; }

    // ============================================================================
    // Modification Interface
    // ============================================================================

    /**
     * @brief Adds a vertex with optional attributes.
     * @note Colors, normals, and texcoords are optional. If not provided,
     *       the arrays won't be extended (use with care for consistency).
     */
    void addVertex(const Vec3& position);
    void addVertex(const Vec3& position, const Vec3& color);
    void addVertex(const Vec3& position, const Vec3& color, const Vec2& texCoord);
    void addVertex(const Vec3& position, const Vec3& color, const Vec2& texCoord, const Vec3& normal);

    /**
     * @brief Adds an index (for indexed rendering).
     */
    void addIndex(Index index);

    /**
     * @brief Adds a triangle by indices.
     */
    void addTriangle(Index i0, Index i1, Index i2);

    /**
     * @brief Reserves capacity to avoid reallocations during construction.
     */
    void reserve(size_t vertexCount, size_t indexCount);

    /**
     * @brief Clears all data, keeping allocated memory.
     */
    void clear() noexcept;

    /**
     * @brief Clears and deallocates all memory.
     */
    void reset() noexcept;

    // ============================================================================
    // Utility Methods
    // ============================================================================

    /**
     * @brief Checks if the mesh has valid data for rendering.
     * @return true if vertices and indices are present
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Returns the number of vertices.
     */
    [[nodiscard]] size_t getVertexCount() const noexcept { return vertices_.size(); }

    /**
     * @brief Returns the number of indices.
     */
    [[nodiscard]] size_t getIndexCount() const noexcept { return indices_.size(); }

    /**
     * @brief Returns the number of triangles (assumes triangle list topology).
     */
    [[nodiscard]] size_t getTriangleCount() const noexcept { return indices_.size() / 3; }

    /**
     * @brief Checks if mesh has per-vertex colors.
     */
    [[nodiscard]] bool hasColors() const noexcept { return !colors_.empty(); }

    /**
     * @brief Checks if mesh has texture coordinates.
     */
    [[nodiscard]] bool hasTexCoords() const noexcept { return !texCoords_.empty(); }

    /**
     * @brief Checks if mesh has normals.
     */
    [[nodiscard]] bool hasNormals() const noexcept { return !normals_.empty(); }

    /**
     * @brief Computes and stores flat normals (one per triangle, duplicated per vertex).
     * @note This will resize the normals array to match vertex count.
     */
    void computeFlatNormals();

    /**
     * @brief Computes and stores smooth normals (averaged per vertex).
     * @note Requires indices to be set up correctly.
     */
    void computeSmoothNormals();

    /**
     * @brief Validates that all attribute arrays have consistent sizes.
     * @return true if mesh data is consistent
     */
    [[nodiscard]] bool validate() const noexcept;

private:
    // ============================================================================
    // Member Data - All owned by value
    // ============================================================================

    std::vector<Vec3> vertices_;    // Required: positions
    std::vector<Index> indices_;    // Required: index buffer for indexed drawing
    std::vector<Vec3> colors_;      // Optional: per-vertex colors (RGB)
    std::vector<Vec2> texCoords_;   // Optional: texture coordinates (UV)
    std::vector<Vec3> normals_;     // Optional: per-vertex normals (for lighting)

    // Note: Could extend with tangents/bitangents for normal mapping if needed
};

// ============================================================================
// Factory Functions for Common Primitives
// ============================================================================

namespace MeshFactory {
    /**
     * @brief Creates a unit cube centered at origin.
     */
    [[nodiscard]] std::shared_ptr<Graphics::Mesh> createCube(float size = 1.0f);

    /**
     * @brief Creates a UV sphere.
     */
    [[nodiscard]] Mesh createSphere(float radius = 1.0f, uint32_t segments = 32, uint32_t rings = 16);

    /**
     * @brief Creates a plane on the XZ plane.
     */
    [[nodiscard]] Mesh createPlane(float width = 1.0f, float height = 1.0f, uint32_t subdivisionsX = 1, uint32_t subdivisionsZ = 1);

    /**
     * @brief Creates a cylinder along the Y axis.
     */
    [[nodiscard]] Mesh createCylinder(float radius = 1.0f, float height = 2.0f, uint32_t segments = 32);
}

} // namespace Graphics
