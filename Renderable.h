#pragma once

#include "Mesh.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace Graphics {

// Forward declarations for GPU resource types
// These would be defined in your rendering backend (OpenGL, Vulkan, DirectX, etc.)
class Texture;
class Shader;
class Material;
class VertexBuffer;
class IndexBuffer;

/**
 * @brief Represents a complete, renderable object with all GPU resources.
 *
 * A Renderable combines geometry (Mesh) with rendering resources (textures, shaders,
 * materials) needed to draw the object. This class manages GPU resources using RAII
 * and smart pointers.
 *
 * Design rationale:
 * - Separation of concerns: Mesh (data) vs Renderable (presentation)
 * - Resource sharing: Multiple Renderables can share the same Mesh/Texture/Shader
 * - RAII: Automatic GPU resource cleanup via smart pointers
 * - Flexibility: Optional components for different rendering needs
 * - Move-only semantics prevent accidental expensive copies
 *
 * Ownership model:
 * - Mesh: Can be shared (shared_ptr) or owned uniquely based on use case
 * - Textures: Typically shared across multiple objects
 * - Shaders: Almost always shared (expensive to compile)
 * - GPU buffers: Owned uniquely per Renderable (vertex/index buffers)
 */
class Renderable {
public:
    // ============================================================================
    // Type Aliases
    // ============================================================================

    using TexturePtr = std::shared_ptr<Texture>;
    using ShaderPtr = std::shared_ptr<Shader>;
    using MaterialPtr = std::shared_ptr<Material>;
    using MeshPtr = std::shared_ptr<Mesh>;

    // ============================================================================
    // Constructors and Rule of Five
    // ============================================================================

    /**
     * @brief Default constructor creates an empty renderable.
     */
    Renderable() = default;

    /**
     * @brief Constructs a Renderable with a mesh (most common case).
     * @param mesh Shared pointer to mesh data
     * @note GPU buffers are not created until uploadToGPU() is called
     */
    explicit Renderable(MeshPtr mesh);

    /**
     * @brief Constructs a Renderable with all primary resources.
     * @param mesh Shared pointer to mesh data
     * @param shader Shared pointer to shader program
     * @param texture Optional texture (can be nullptr)
     */
    Renderable(MeshPtr mesh, ShaderPtr shader, TexturePtr texture = nullptr);

    /**
     * @brief Destructor - automatically cleans up GPU resources via RAII.
     */
    ~Renderable();

    // Disable copying (GPU resources shouldn't be accidentally copied)
    Renderable(const Renderable&) = delete;
    Renderable& operator=(const Renderable&) = delete;

    // Enable moving (transfer ownership of GPU resources)
    Renderable(Renderable&&) noexcept = default;
    Renderable& operator=(Renderable&&) noexcept = default;

    // ============================================================================
    // Resource Management
    // ============================================================================

    /**
     * @brief Sets or replaces the mesh.
     * @param mesh Shared pointer to mesh data
     * @note Marks GPU buffers as dirty - call uploadToGPU() to sync
     */
    void setMesh(MeshPtr mesh);

    /**
     * @brief Gets the current mesh (may be nullptr).
     */
    [[nodiscard]] const MeshPtr& getMesh() const noexcept { return mesh_; }

    /**
     * @brief Sets the primary shader program.
     */
    void setShader(ShaderPtr shader) { shader_ = std::move(shader); }

    /**
     * @brief Gets the current shader.
     */
    [[nodiscard]] const ShaderPtr& getShader() const noexcept { return shader_; }

    /**
     * @brief Sets the primary texture (typically diffuse/albedo).
     */
    void setTexture(TexturePtr texture) { texture_ = std::move(texture); }

    /**
     * @brief Gets the primary texture.
     */
    [[nodiscard]] const TexturePtr& getTexture() const noexcept { return texture_; }

    /**
     * @brief Sets a named texture (e.g., "normalMap", "specularMap").
     * @param name Texture slot name
     * @param texture Texture to bind
     */
    void setTexture(const std::string& name, TexturePtr texture);

    /**
     * @brief Gets a named texture.
     * @return Texture if found, nullptr otherwise
     */
    [[nodiscard]] TexturePtr getTexture(const std::string& name) const;

    /**
     * @brief Sets the material (contains shader parameters, textures, etc.).
     */
    void setMaterial(MaterialPtr material) { material_ = std::move(material); }

    /**
     * @brief Gets the current material.
     */
    [[nodiscard]] const MaterialPtr& getMaterial() const noexcept { return material_; }

    // ============================================================================
    // GPU Resource Management
    // ============================================================================

    /**
     * @brief Uploads mesh data to GPU buffers.
     * @note Call this after setting/modifying the mesh or when GPU context changes.
     * @throws std::runtime_error if mesh is null or invalid
     */
    void uploadToGPU();

    /**
     * @brief Checks if GPU buffers are up-to-date with CPU mesh data.
     */
    [[nodiscard]] bool isUploadedToGPU() const noexcept { return gpuDataValid_; }

    /**
     * @brief Marks GPU data as stale (needs re-upload).
     * @note Automatically called when mesh is changed
     */
    void invalidateGPUData() noexcept { gpuDataValid_ = false; }

    /**
     * @brief Frees GPU resources (VBO, IBO, etc.).
     * @note Useful for manual resource management or context loss
     */
    void releaseGPUResources();

    // ============================================================================
    // Rendering Interface
    // ============================================================================

    /**
     * @brief Renders the object using the currently bound shader and state.
     * @note Assumes render state (viewport, blend mode, etc.) is set externally
     * @throws std::runtime_error if not ready to render (no mesh, not uploaded, etc.)
     */
    void render() const;

    /**
     * @brief Renders with a specific shader (overrides the default shader).
     * @param shader Shader to use for this draw call
     */
    void render(const Shader& shader) const;

    /**
     * @brief Checks if the renderable is ready to be drawn.
     * @return true if has valid mesh, GPU buffers are uploaded, and has shader
     */
    [[nodiscard]] bool isReadyToRender() const noexcept;

    // ============================================================================
    // State Management
    // ============================================================================

    /**
     * @brief Enables/disables this renderable for rendering.
     */
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    /**
     * @brief Checks if this renderable is enabled.
     */
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
     * @brief Sets whether this object casts shadows.
     */
    void setCastsShadows(bool casts) noexcept { castsShadows_ = casts; }

    /**
     * @brief Checks if this object casts shadows.
     */
    [[nodiscard]] bool castsShadows() const noexcept { return castsShadows_; }

    /**
     * @brief Sets whether this object receives shadows.
     */
    void setReceivesShadows(bool receives) noexcept { receivesShadows_ = receives; }

    /**
     * @brief Checks if this object receives shadows.
     */
    [[nodiscard]] bool receivesShadows() const noexcept { return receivesShadows_; }

    // ============================================================================
    // Advanced Features
    // ============================================================================

    /**
     * @brief Sets custom shader parameters (uniforms).
     * @param name Parameter name
     * @param value Parameter value (type-erased, typically float, vec3, mat4, etc.)
     */
    template<typename T>
    void setParameter(const std::string& name, const T& value);

    /**
     * @brief Gets a custom parameter value.
     */
    template<typename T>
    [[nodiscard]] std::optional<T> getParameter(const std::string& name) const;

    /**
     * @brief Binds all textures and prepares state for rendering.
     * @note Called internally by render(), but can be used manually
     */
    void bind() const;

    /**
     * @brief Unbinds resources after rendering.
     */
    void unbind() const;

    // ============================================================================
    // Debug and Utilities
    // ============================================================================

    /**
     * @brief Gets a human-readable description of this renderable.
     */
    [[nodiscard]] std::string getDebugInfo() const;

    /**
     * @brief Validates that all required resources are present and valid.
     */
    [[nodiscard]] bool validate() const noexcept;

private:
    // ============================================================================
    // Core Resources - Shared ownership
    // ============================================================================

    MeshPtr mesh_;              // Geometry data (can be shared)
    ShaderPtr shader_;          // Shader program (typically shared)
    TexturePtr texture_;        // Primary texture (typically shared)
    MaterialPtr material_;      // Material properties (can be shared or unique)

    // Additional textures for complex materials (normal maps, etc.)
    std::unordered_map<std::string, TexturePtr> additionalTextures_;

    // ============================================================================
    // GPU Resources - Unique ownership (handle types depend on API)
    // ============================================================================

    // These would be unique_ptr in a real implementation, pointing to
    // platform-specific buffer objects (OpenGL VBO/VAO/IBO, Vulkan buffers, etc.)
    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;

    // Platform-specific vertex array object or input layout
    // uint32_t vaoHandle_ = 0; // Example for OpenGL

    // ============================================================================
    // State Flags
    // ============================================================================

    bool gpuDataValid_ = false;      // True if GPU buffers match CPU mesh
    bool enabled_ = true;            // Whether to render this object
    bool castsShadows_ = true;       // Shadow casting flag
    bool receivesShadows_ = true;    // Shadow receiving flag

    // ============================================================================
    // Custom Parameters (for shader uniforms, etc.)
    // ============================================================================

    // In a real implementation, you'd use a type-erased container like std::any
    // or a custom variant type for shader parameters
    // std::unordered_map<std::string, ShaderParameter> parameters_;
};

// ============================================================================
// Alternative Names and Use Cases
// ============================================================================

// Type alias for alternative naming conventions:
using Model = Renderable;           // Common in game engines
using RenderObject = Renderable;    // Common in rendering engines
using Drawable = Renderable;        // Common in scene graphs

/**
 * ARCHITECTURAL NOTES:
 *
 * 1. Mesh vs Renderable Separation:
 *    - Mesh: Pure data, lightweight, copyable, no GPU knowledge
 *    - Renderable: Complete rendering context, GPU resources, move-only
 *
 * 2. Resource Sharing:
 *    - Use shared_ptr for resources that are expensive and shareable
 *    - Use unique_ptr for per-object GPU buffers
 *    - Multiple Renderables can reference the same Mesh (instancing)
 *
 * 3. GPU Upload Strategy:
 *    - Explicit uploadToGPU() gives control over when uploads happen
 *    - Dirty flag prevents redundant uploads
 *    - Alternative: Automatic upload on first render() call
 *
 * 4. Extensibility:
 *    - Easy to add: LOD system, animation data, physics properties
 *    - Material system can be expanded with PBR parameters
 *    - Custom parameters allow per-object shader customization
 *
 * 5. Performance Considerations:
 *    - Move semantics avoid expensive copies
 *    - Shared resources reduce memory footprint
 *    - Explicit GPU management avoids stalls
 *    - Batch-friendly design (sort by shader/texture before rendering)
 */

} // namespace Graphics
