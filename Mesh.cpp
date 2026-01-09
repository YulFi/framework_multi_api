#include "Mesh.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace Graphics {

// ============================================================================
// Mesh Implementation
// ============================================================================

Mesh::Mesh(size_t vertexCount, size_t indexCount) {
    reserve(vertexCount, indexCount);
}

void Mesh::addVertex(const Vec3& position) {
    vertices_.push_back(position);
}

void Mesh::addVertex(const Vec3& position, const Vec3& color) {
    vertices_.push_back(position);
    colors_.push_back(color);
}

void Mesh::addVertex(const Vec3& position, const Vec3& color, const Vec2& texCoord) {
    vertices_.push_back(position);
    colors_.push_back(color);
    texCoords_.push_back(texCoord);
}

void Mesh::addVertex(const Vec3& position, const Vec3& color, const Vec2& texCoord, const Vec3& normal) {
    vertices_.push_back(position);
    colors_.push_back(color);
    texCoords_.push_back(texCoord);
    normals_.push_back(normal);
}

void Mesh::addIndex(Index index) {
    indices_.push_back(index);
}

void Mesh::addTriangle(Index i0, Index i1, Index i2) {
    indices_.push_back(i0);
    indices_.push_back(i1);
    indices_.push_back(i2);
}

void Mesh::reserve(size_t vertexCount, size_t indexCount) {
    vertices_.reserve(vertexCount);
    indices_.reserve(indexCount);
    // Optionally reserve for other attributes if you know they'll be used
}

void Mesh::clear() noexcept {
    vertices_.clear();
    indices_.clear();
    colors_.clear();
    texCoords_.clear();
    normals_.clear();
}

void Mesh::reset() noexcept {
    vertices_ = std::vector<Vec3>();
    indices_ = std::vector<Index>();
    colors_ = std::vector<Vec3>();
    texCoords_ = std::vector<Vec2>();
    normals_ = std::vector<Vec3>();
}

bool Mesh::isValid() const noexcept {
    return !vertices_.empty() && !indices_.empty();
}

bool Mesh::validate() const noexcept {
    // Check that all attribute arrays are either empty or match vertex count
    const size_t vertCount = vertices_.size();

    if (vertCount == 0) {
        return false; // Must have vertices
    }

    if (!colors_.empty() && colors_.size() != vertCount) {
        return false;
    }

    if (!texCoords_.empty() && texCoords_.size() != vertCount) {
        return false;
    }

    if (!normals_.empty() && normals_.size() != vertCount) {
        return false;
    }

    // Check that indices are within valid range
    if (!indices_.empty()) {
        const auto maxIndex = *std::max_element(indices_.begin(), indices_.end());
        if (maxIndex >= vertCount) {
            return false; // Index out of bounds
        }
    }

    // Check that index count is multiple of 3 (assuming triangles)
    if (indices_.size() % 3 != 0) {
        return false;
    }

    return true;
}

void Mesh::computeFlatNormals() {
    if (indices_.size() < 3 || vertices_.size() < 3) {
        return; // Not enough data
    }

    // Resize normals to match vertices
    normals_.resize(vertices_.size());

    // Compute normal for each triangle and assign to all vertices of that triangle
    for (size_t i = 0; i < indices_.size(); i += 3) {
        const Index i0 = indices_[i];
        const Index i1 = indices_[i + 1];
        const Index i2 = indices_[i + 2];

        const Vec3& v0 = vertices_[i0];
        const Vec3& v1 = vertices_[i1];
        const Vec3& v2 = vertices_[i2];

        // Compute triangle edges
        const Vec3 edge1{v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
        const Vec3 edge2{v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

        // Cross product for normal
        Vec3 normal{
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x
        };

        // Normalize
        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 1e-6f) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }

        // Assign to all three vertices (flat shading)
        normals_[i0] = normal;
        normals_[i1] = normal;
        normals_[i2] = normal;
    }
}

void Mesh::computeSmoothNormals() {
    if (indices_.size() < 3 || vertices_.size() < 3) {
        return;
    }

    // Initialize normals to zero
    normals_.assign(vertices_.size(), Vec3{0.0f, 0.0f, 0.0f});

    // Accumulate normals from all triangles
    for (size_t i = 0; i < indices_.size(); i += 3) {
        const Index i0 = indices_[i];
        const Index i1 = indices_[i + 1];
        const Index i2 = indices_[i + 2];

        const Vec3& v0 = vertices_[i0];
        const Vec3& v1 = vertices_[i1];
        const Vec3& v2 = vertices_[i2];

        // Compute triangle edges
        const Vec3 edge1{v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
        const Vec3 edge2{v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

        // Cross product for normal
        const Vec3 normal{
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x
        };

        // Accumulate (weighted by triangle area implicitly)
        normals_[i0].x += normal.x;
        normals_[i0].y += normal.y;
        normals_[i0].z += normal.z;

        normals_[i1].x += normal.x;
        normals_[i1].y += normal.y;
        normals_[i1].z += normal.z;

        normals_[i2].x += normal.x;
        normals_[i2].y += normal.y;
        normals_[i2].z += normal.z;
    }

    // Normalize all accumulated normals
    for (auto& normal : normals_) {
        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 1e-6f) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }
    }
}

// ============================================================================
// Mesh Factory Functions
// ============================================================================

namespace MeshFactory {

    std::shared_ptr<Graphics::Mesh> createCube(float size) {
    const float half = size * 0.5f;

    std::shared_ptr<Graphics::Mesh> mesh = std::make_shared<Graphics::Mesh>();
    mesh->reserve(24, 36); // 6 faces * 4 vertices, 6 faces * 2 triangles * 3 indices

    // Define 8 unique positions
    const Mesh::Vec3 positions[8] = {
        {-half, -half, -half}, // 0: left-bottom-back
        { half, -half, -half}, // 1: right-bottom-back
        { half,  half, -half}, // 2: right-top-back
        {-half,  half, -half}, // 3: left-top-back
        {-half, -half,  half}, // 4: left-bottom-front
        { half, -half,  half}, // 5: right-bottom-front
        { half,  half,  half}, // 6: right-top-front
        {-half,  half,  half}, // 7: left-top-front
    };

    // Front face (z+)
    mesh->addVertex(positions[4], {1, 0, 0}, {0, 0}, {0, 0, 1});
    mesh->addVertex(positions[5], {1, 0, 0}, {1, 0}, {0, 0, 1});
    mesh->addVertex(positions[6], {1, 0, 0}, {1, 1}, {0, 0, 1});
    mesh->addVertex(positions[7], {1, 0, 0}, {0, 1}, {0, 0, 1});
    mesh->addTriangle(0, 1, 2);
    mesh->addTriangle(0, 2, 3);

    // Back face (z-)
    mesh->addVertex(positions[1], {0, 1, 0}, {0, 0}, {0, 0, -1});
    mesh->addVertex(positions[0], {0, 1, 0}, {1, 0}, {0, 0, -1});
    mesh->addVertex(positions[3], {0, 1, 0}, {1, 1}, {0, 0, -1});
    mesh->addVertex(positions[2], {0, 1, 0}, {0, 1}, {0, 0, -1});
    mesh->addTriangle(4, 5, 6);
    mesh->addTriangle(4, 6, 7);

    // Right face (x+)
    mesh->addVertex(positions[5], {0, 0, 1}, {0, 0}, {1, 0, 0});
    mesh->addVertex(positions[1], {0, 0, 1}, {1, 0}, {1, 0, 0});
    mesh->addVertex(positions[2], {0, 0, 1}, {1, 1}, {1, 0, 0});
    mesh->addVertex(positions[6], {0, 0, 1}, {0, 1}, {1, 0, 0});
    mesh->addTriangle(8, 9, 10);
    mesh->addTriangle(8, 10, 11);

    // Left face (x-)
    mesh->addVertex(positions[0], {1, 1, 0}, {0, 0}, {-1, 0, 0});
    mesh->addVertex(positions[4], {1, 1, 0}, {1, 0}, {-1, 0, 0});
    mesh->addVertex(positions[7], {1, 1, 0}, {1, 1}, {-1, 0, 0});
    mesh->addVertex(positions[3], {1, 1, 0}, {0, 1}, {-1, 0, 0});
    mesh->addTriangle(12, 13, 14);
    mesh->addTriangle(12, 14, 15);

    // Top face (y+)
    mesh->addVertex(positions[3], {1, 0, 1}, {0, 0}, {0, 1, 0});
    mesh->addVertex(positions[7], {1, 0, 1}, {1, 0}, {0, 1, 0});
    mesh->addVertex(positions[6], {1, 0, 1}, {1, 1}, {0, 1, 0});
    mesh->addVertex(positions[2], {1, 0, 1}, {0, 1}, {0, 1, 0});
    mesh->addTriangle(16, 17, 18);
    mesh->addTriangle(16, 18, 19);

    // Bottom face (y-)
    mesh->addVertex(positions[4], {0, 1, 1}, {0, 0}, {0, -1, 0});
    mesh->addVertex(positions[0], {0, 1, 1}, {1, 0}, {0, -1, 0});
    mesh->addVertex(positions[1], {0, 1, 1}, {1, 1}, {0, -1, 0});
    mesh->addVertex(positions[5], {0, 1, 1}, {0, 1}, {0, -1, 0});
    mesh->addTriangle(20, 21, 22);
    mesh->addTriangle(20, 22, 23);

    return mesh;
}

Mesh createSphere(float radius, uint32_t segments, uint32_t rings) {
    Mesh mesh;

    const float pi = 3.14159265359f;
    const uint32_t vertexCount = (rings + 1) * (segments + 1);
    const uint32_t indexCount = rings * segments * 6;

    mesh.reserve(vertexCount, indexCount);

    // Generate vertices
    for (uint32_t ring = 0; ring <= rings; ++ring) {
        const float phi = pi * static_cast<float>(ring) / static_cast<float>(rings);
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (uint32_t seg = 0; seg <= segments; ++seg) {
            const float theta = 2.0f * pi * static_cast<float>(seg) / static_cast<float>(segments);
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const Mesh::Vec3 normal{sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
            const Mesh::Vec3 position{radius * normal.x, radius * normal.y, radius * normal.z};
            const Mesh::Vec2 texCoord{
                static_cast<float>(seg) / static_cast<float>(segments),
                static_cast<float>(ring) / static_cast<float>(rings)
            };
            const Mesh::Vec3 color{1.0f, 1.0f, 1.0f};

            mesh.addVertex(position, color, texCoord, normal);
        }
    }

    // Generate indices
    for (uint32_t ring = 0; ring < rings; ++ring) {
        for (uint32_t seg = 0; seg < segments; ++seg) {
            const uint32_t current = ring * (segments + 1) + seg;
            const uint32_t next = current + segments + 1;

            mesh.addTriangle(current, next, current + 1);
            mesh.addTriangle(current + 1, next, next + 1);
        }
    }

    return mesh;
}

Mesh createPlane(float width, float height, uint32_t subdivisionsX, uint32_t subdivisionsZ) {
    Mesh mesh;

    const uint32_t verticesX = subdivisionsX + 1;
    const uint32_t verticesZ = subdivisionsZ + 1;
    const uint32_t vertexCount = verticesX * verticesZ;
    const uint32_t indexCount = subdivisionsX * subdivisionsZ * 6;

    mesh.reserve(vertexCount, indexCount);

    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    // Generate vertices
    for (uint32_t z = 0; z < verticesZ; ++z) {
        for (uint32_t x = 0; x < verticesX; ++x) {
            const float px = -halfWidth + (width * x) / subdivisionsX;
            const float pz = -halfHeight + (height * z) / subdivisionsZ;

            const Mesh::Vec3 position{px, 0.0f, pz};
            const Mesh::Vec3 normal{0.0f, 1.0f, 0.0f};
            const Mesh::Vec2 texCoord{
                static_cast<float>(x) / subdivisionsX,
                static_cast<float>(z) / subdivisionsZ
            };
            const Mesh::Vec3 color{1.0f, 1.0f, 1.0f};

            mesh.addVertex(position, color, texCoord, normal);
        }
    }

    // Generate indices
    for (uint32_t z = 0; z < subdivisionsZ; ++z) {
        for (uint32_t x = 0; x < subdivisionsX; ++x) {
            const uint32_t topLeft = z * verticesX + x;
            const uint32_t topRight = topLeft + 1;
            const uint32_t bottomLeft = (z + 1) * verticesX + x;
            const uint32_t bottomRight = bottomLeft + 1;

            mesh.addTriangle(topLeft, bottomLeft, topRight);
            mesh.addTriangle(topRight, bottomLeft, bottomRight);
        }
    }

    return mesh;
}

Mesh createCylinder(float radius, float height, uint32_t segments) {
    Mesh mesh;

    const float pi = 3.14159265359f;
    const float halfHeight = height * 0.5f;

    // Approximate vertex count: side vertices + top cap + bottom cap + centers
    const uint32_t vertexCount = (segments * 2) + (segments * 2) + 2;
    const uint32_t indexCount = (segments * 6) + (segments * 3) + (segments * 3);

    mesh.reserve(vertexCount, indexCount);

    // Side vertices
    for (uint32_t i = 0; i <= segments; ++i) {
        const float theta = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        const Mesh::Vec3 normal{cosTheta, 0.0f, sinTheta};
        const Mesh::Vec2 texCoord{static_cast<float>(i) / segments, 0.0f};

        // Bottom vertex
        mesh.addVertex({radius * cosTheta, -halfHeight, radius * sinTheta}, {1, 1, 1}, texCoord, normal);

        // Top vertex
        mesh.addVertex({radius * cosTheta, halfHeight, radius * sinTheta}, {1, 1, 1},
                      {texCoord.x, 1.0f}, normal);
    }

    // Generate side indices
    for (uint32_t i = 0; i < segments; ++i) {
        const uint32_t bottomCurrent = i * 2;
        const uint32_t topCurrent = bottomCurrent + 1;
        const uint32_t bottomNext = (i + 1) * 2;
        const uint32_t topNext = bottomNext + 1;

        mesh.addTriangle(bottomCurrent, bottomNext, topCurrent);
        mesh.addTriangle(topCurrent, bottomNext, topNext);
    }

    // Top and bottom caps (simplified - you'd add proper cap geometry here)

    return mesh;
}

} // namespace MeshFactory

} // namespace Graphics
