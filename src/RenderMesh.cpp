#include "RenderMesh.h"
#include "Logger.h"
#include <stdexcept>
#include <algorithm>

namespace Graphics {

// ============================================================================
// Construction
// ============================================================================

RenderMesh::RenderMesh(
    const Mesh& mesh,
    IRenderer& renderer,
    BufferUsage usage,
    PrimitiveType primitiveType
)
    : renderer_(&renderer)
    , vertexCount_(mesh.getVertexCount())
    , indexCount_(mesh.getIndexCount())
    , bufferUsage_(usage)
    , primitiveType_(primitiveType)
    , hasColors_(mesh.hasColors())
    , hasTexCoords_(mesh.hasTexCoords())
    , hasNormals_(mesh.hasNormals())
{
    // Validate input mesh
    validateMesh(mesh);

    // Create GPU resources via renderer factory methods
    vertexArray_ = renderer.createVertexArray();
    vertexBuffer_ = renderer.createVertexBuffer();
    indexBuffer_ = renderer.createIndexBuffer();

    if (!vertexArray_ || !vertexBuffer_ || !indexBuffer_) {
        throw std::runtime_error("Failed to create GPU resources for RenderMesh");
    }

    // Bind VAO first - subsequent buffer bindings will be captured by the VAO
    vertexArray_->bind();

    // ========================================================================
    // Interleave and upload vertex data
    // ========================================================================

    std::vector<float> interleavedData;
    interleaveVertexData(mesh, interleavedData);

    const size_t vertexDataSize = interleavedData.size() * sizeof(float);

    vertexBuffer_->bind();
    vertexBuffer_->setData(interleavedData.data(), vertexDataSize, usage);

    // ========================================================================
    // Set up vertex attributes
    // ========================================================================

    const size_t stride = getVertexStride();
    setupVertexAttributes(stride);

    // ========================================================================
    // Upload index data
    // ========================================================================

    const auto& indices = mesh.getIndices();

    // Determine appropriate index type based on vertex count
    // Note: UnsignedByte (UINT8) is not supported in Vulkan without extension
    // So we use UnsignedShort as the minimum
    IndexType indexType;
    if (vertexCount_ <= 0xFFFF) {
        indexType = IndexType::UnsignedShort;  // UINT16
    } else {
        indexType = IndexType::UnsignedInt;    // UINT32
    }

    indexBuffer_->bind();
    indexBuffer_->setData(indices.data(), indexCount_, indexType, usage);

    // Unbind to prevent accidental modification
    vertexArray_->unbind();
    vertexBuffer_->unbind();
    indexBuffer_->unbind();
}

// ============================================================================
// Rendering Interface
// ============================================================================

void RenderMesh::draw() const {
    draw(primitiveType_);
}

void RenderMesh::draw(PrimitiveType primitiveType) const {
    static bool firstDraw = true;
    if (firstDraw) {
        LOG_INFO("RenderMesh::draw() - First draw call");
        LOG_INFO("  vertexCount: {}, indexCount: {}", vertexCount_, indexCount_);
        firstDraw = false;
    }

    // Bind VAO (automatically binds associated VBO and IBO)
    vertexArray_->bind();

    // Issue indexed draw call
    // Note: IIndexBuffer stores its type, so we don't need to query it explicitly
    // The renderer's drawElements will use the bound index buffer's metadata
    renderer_->drawElements(
        primitiveType,
        static_cast<int>(indexCount_),
        0,  // Index type is stored in IIndexBuffer
        nullptr // Using bound index buffer, not client-side pointer
    );

    // Note: VAO remains bound - caller can unbind if needed for state management
}

void RenderMesh::drawSubset(size_t indexCount, size_t indexOffset) const {
    if (indexOffset + indexCount > indexCount_) {
        throw std::out_of_range(
            "RenderMesh::drawSubset: requested range exceeds index buffer size"
        );
    }

    vertexArray_->bind();

    // Calculate byte offset based on index type
    const void* offset = reinterpret_cast<const void*>(
        indexOffset * sizeof(Mesh::Index)
    );

    renderer_->drawElements(
        primitiveType_,
        static_cast<int>(indexCount),
        0,  // Index type from IIndexBuffer
        offset
    );
}

// ============================================================================
// Update Interface
// ============================================================================

void RenderMesh::update(const Mesh& mesh) {
    validateMesh(mesh);

    // Check if vertex layout changed
    if (mesh.hasColors() != hasColors_ ||
        mesh.hasTexCoords() != hasTexCoords_ ||
        mesh.hasNormals() != hasNormals_) {
        throw std::runtime_error(
            "RenderMesh::update: vertex layout changed. Cannot update mesh with different attributes."
        );
    }

    // Update metadata
    const size_t newVertexCount = mesh.getVertexCount();
    const size_t newIndexCount = mesh.getIndexCount();

    // ========================================================================
    // Update vertex buffer
    // ========================================================================

    std::vector<float> interleavedData;
    interleaveVertexData(mesh, interleavedData);

    const size_t vertexDataSize = interleavedData.size() * sizeof(float);

    vertexBuffer_->bind();

    if (newVertexCount == vertexCount_) {
        // Same size - use updateData for efficiency
        vertexBuffer_->updateData(interleavedData.data(), vertexDataSize, 0);
    } else {
        // Size changed - need to reallocate
        vertexBuffer_->setData(interleavedData.data(), vertexDataSize, bufferUsage_);
        vertexCount_ = newVertexCount;
    }

    // ========================================================================
    // Update index buffer
    // ========================================================================

    const auto& indices = mesh.getIndices();

    indexBuffer_->bind();

    if (newIndexCount == indexCount_) {
        // Same size - use updateData for efficiency
        indexBuffer_->updateData(indices.data(), newIndexCount, 0);
    } else {
        // Size changed - need to reallocate
        IndexType indexType;
        if (newVertexCount <= 0xFF) {
            indexType = IndexType::UnsignedByte;
        } else if (newVertexCount <= 0xFFFF) {
            indexType = IndexType::UnsignedShort;
        } else {
            indexType = IndexType::UnsignedInt;
        }

        indexBuffer_->setData(indices.data(), newIndexCount, indexType, bufferUsage_);
        indexCount_ = newIndexCount;
    }

    vertexBuffer_->unbind();
    indexBuffer_->unbind();
}

void RenderMesh::updateVertexData(const Mesh& mesh) {
    if (mesh.getVertexCount() != vertexCount_) {
        throw std::invalid_argument(
            "RenderMesh::updateVertexData: vertex count changed. Use update() instead."
        );
    }

    // Validate layout hasn't changed
    if (mesh.hasColors() != hasColors_ ||
        mesh.hasTexCoords() != hasTexCoords_ ||
        mesh.hasNormals() != hasNormals_) {
        throw std::runtime_error(
            "RenderMesh::updateVertexData: vertex layout changed"
        );
    }

    // Interleave and update vertex data only
    std::vector<float> interleavedData;
    interleaveVertexData(mesh, interleavedData);

    const size_t vertexDataSize = interleavedData.size() * sizeof(float);

    vertexBuffer_->bind();
    vertexBuffer_->updateData(interleavedData.data(), vertexDataSize, 0);
    vertexBuffer_->unbind();
}

// ============================================================================
// State Query
// ============================================================================

size_t RenderMesh::getVertexStride() const noexcept {
    size_t stride = sizeof(float) * 3; // Position (always present)

    if (hasColors_) {
        stride += sizeof(float) * 3; // Color RGB
    }

    if (hasTexCoords_) {
        stride += sizeof(float) * 2; // TexCoord UV
    }

    if (hasNormals_) {
        stride += sizeof(float) * 3; // Normal XYZ
    }

    return stride;
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

void RenderMesh::interleaveVertexData(const Mesh& mesh, std::vector<float>& outBuffer) const {
    const auto& vertices = mesh.getVertices();
    const auto& colors = mesh.getColors();
    const auto& texCoords = mesh.getTexCoords();
    const auto& normals = mesh.getNormals();

    const size_t vertCount = vertices.size();

    // Calculate floats per vertex
    size_t floatsPerVertex = 3; // position
    if (hasColors_)    floatsPerVertex += 3;
    if (hasTexCoords_) floatsPerVertex += 2;
    if (hasNormals_)   floatsPerVertex += 3;

    // Reserve space for efficiency
    outBuffer.clear();
    outBuffer.reserve(vertCount * floatsPerVertex);

    // Interleave data
    for (size_t i = 0; i < vertCount; ++i) {
        // Position (always present)
        const auto& pos = vertices[i];
        outBuffer.push_back(pos.x);
        outBuffer.push_back(pos.y);
        outBuffer.push_back(pos.z);

        // Color (optional)
        if (hasColors_) {
            const auto& col = colors[i];
            outBuffer.push_back(col.x);
            outBuffer.push_back(col.y);
            outBuffer.push_back(col.z);
        }

        // TexCoord (optional)
        if (hasTexCoords_) {
            const auto& tex = texCoords[i];
            outBuffer.push_back(tex.x);
            outBuffer.push_back(tex.y);
        }

        // Normal (optional)
        if (hasNormals_) {
            const auto& norm = normals[i];
            outBuffer.push_back(norm.x);
            outBuffer.push_back(norm.y);
            outBuffer.push_back(norm.z);
        }
    }
}

void RenderMesh::setupVertexAttributes(size_t stride) {
    size_t offset = 0;

    // Position attribute (location 0) - always present
    vertexArray_->addAttribute(VertexAttribute(
        ATTRIB_POSITION,
        3,                      // size: 3 components (x, y, z)
        DataType::Float,
        false,                  // normalized: false
        stride,
        reinterpret_cast<const void*>(offset)
    ));
    offset += sizeof(float) * 3;

    // Color attribute (location 1) - optional
    if (hasColors_) {
        vertexArray_->addAttribute(VertexAttribute(
            ATTRIB_COLOR,
            3,                  // size: 3 components (r, g, b)
            DataType::Float,
            false,              // normalized: false (colors are 0-1 floats)
            stride,
            reinterpret_cast<const void*>(offset)
        ));
        offset += sizeof(float) * 3;
    }

    // TexCoord attribute (location 2) - optional
    if (hasTexCoords_) {
        vertexArray_->addAttribute(VertexAttribute(
            ATTRIB_TEXCOORD,
            2,                  // size: 2 components (u, v)
            DataType::Float,
            false,              // normalized: false
            stride,
            reinterpret_cast<const void*>(offset)
        ));
        offset += sizeof(float) * 2;
    }

    // Normal attribute (location 3) - optional
    if (hasNormals_) {
        vertexArray_->addAttribute(VertexAttribute(
            ATTRIB_NORMAL,
            3,                  // size: 3 components (x, y, z)
            DataType::Float,
            false,              // normalized: false (normals should already be normalized)
            stride,
            reinterpret_cast<const void*>(offset)
        ));
        offset += sizeof(float) * 3;
    }
}

void RenderMesh::validateMesh(const Mesh& mesh) const {
    // Check that mesh has required data
    if (mesh.getVertexCount() == 0) {
        throw std::invalid_argument("RenderMesh: mesh has no vertices");
    }

    if (mesh.getIndexCount() == 0) {
        throw std::invalid_argument("RenderMesh: mesh has no indices");
    }

    // Validate that attribute arrays are consistent
    if (!mesh.validate()) {
        throw std::invalid_argument("RenderMesh: mesh validation failed (inconsistent attribute sizes)");
    }

    // Check for degenerate triangles (if using triangle primitive)
    if (primitiveType_ == PrimitiveType::Triangles && mesh.getIndexCount() % 3 != 0) {
        throw std::invalid_argument("RenderMesh: index count must be multiple of 3 for triangle primitive");
    }
}

} // namespace Graphics
