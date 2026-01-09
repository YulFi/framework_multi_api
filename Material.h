#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declarations for your existing interfaces
class IShaderProgram;
class ITexture;

namespace Graphics {

// Type alias for uniform values - supports common shader uniform types
using UniformValue = std::variant<
    int,
    float,
    bool,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat3,
    glm::mat4
>;

/**
 * @brief Represents a texture binding for a material
 *
 * Stores the relationship between a texture, its sampler uniform name,
 * and the texture unit it should be bound to.
 */
struct TextureBinding {
    std::shared_ptr<ITexture> texture;  // Shared ownership - textures can be reused
    std::string samplerName;            // Uniform name in shader (e.g., "u_DiffuseMap")
    unsigned int textureUnit;           // Texture unit to bind to (0-31 typically)

    TextureBinding(std::shared_ptr<ITexture> tex, std::string name, unsigned int unit)
        : texture(std::move(tex)), samplerName(std::move(name)), textureUnit(unit) {}
};

/**
 * @brief Material class that combines shader, textures, and properties
 *
 * A Material represents the complete visual appearance of a surface by combining:
 * - A shader program (non-owning pointer managed by ShaderManager)
 * - Multiple textures with their sampler bindings
 * - Uniform properties (colors, scalars, vectors, matrices)
 *
 * Design Principles:
 * - RAII: Automatic resource management via smart pointers
 * - Type Safety: Variant-based uniform storage with compile-time checking
 * - Plugin Agnostic: Works with any IShaderProgram/ITexture implementation
 * - Extensible: Virtual methods for custom material behavior
 *
 * Usage Example:
 * @code
 *   auto material = std::make_unique<Material>(shaderProgram);
 *   material->setTexture("u_DiffuseMap", diffuseTexture, 0);
 *   material->setTexture("u_NormalMap", normalTexture, 1);
 *   material->setProperty("u_Color", glm::vec3(1.0f, 0.8f, 0.6f));
 *   material->setProperty("u_Shininess", 32.0f);
 *
 *   // Later during rendering:
 *   material->bind();
 *   // ... draw geometry ...
 *   material->unbind();
 * @endcode
 *
 * Thread Safety: Not thread-safe. Bind/unbind must be called from render thread.
 */
class Material {
public:
    /**
     * @brief Construct a material with a shader program
     * @param shader Non-owning pointer to shader program (managed by ShaderManager)
     * @throws std::invalid_argument if shader is nullptr
     */
    explicit Material(IShaderProgram* shader);

    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~Material() = default;

    // Movable but not copyable (materials own unique configurations)
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    /**
     * @brief Bind the material for rendering
     *
     * Activates the shader program, binds all textures to their respective units,
     * and uploads all material properties as uniforms. Must be called before draw calls.
     *
     * @throws std::runtime_error if shader is null or binding fails
     */
    virtual void bind();

    /**
     * @brief Unbind the material after rendering
     *
     * Unbinds all textures and the shader program. Should be called after draw calls
     * to ensure clean state for next material.
     */
    virtual void unbind();

    /**
     * @brief Check if material is ready for rendering
     * @return true if shader is valid and material can be bound
     */
    [[nodiscard]] bool isValid() const noexcept { return m_shader != nullptr; }

    // ============================================================================
    // Texture Management
    // ============================================================================

    /**
     * @brief Set a texture with explicit sampler name and unit
     * @param samplerName Uniform name in shader (e.g., "u_DiffuseMap")
     * @param texture Shared pointer to texture (can be nullptr to remove)
     * @param textureUnit Texture unit to bind to (0-31, must be unique per material)
     * @throws std::invalid_argument if textureUnit is already in use
     */
    void setTexture(const std::string& samplerName,
                    std::shared_ptr<ITexture> texture,
                    unsigned int textureUnit);

    /**
     * @brief Set a texture with automatic unit assignment
     * @param samplerName Uniform name in shader
     * @param texture Shared pointer to texture
     * @return The assigned texture unit
     *
     * Automatically assigns the next available texture unit. Useful when you
     * don't care about specific unit assignments.
     */
    unsigned int setTexture(const std::string& samplerName,
                           std::shared_ptr<ITexture> texture);

    /**
     * @brief Remove a texture by sampler name
     * @param samplerName The sampler uniform name
     * @return true if texture was found and removed
     */
    bool removeTexture(const std::string& samplerName);

    /**
     * @brief Get a texture by sampler name
     * @param samplerName The sampler uniform name
     * @return Shared pointer to texture, or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<ITexture> getTexture(const std::string& samplerName) const;

    /**
     * @brief Get all texture bindings
     * @return Vector of all texture bindings (read-only)
     */
    [[nodiscard]] const std::vector<TextureBinding>& getTextureBindings() const noexcept {
        return m_textureBindings;
    }

    // ============================================================================
    // Material Properties (Uniforms)
    // ============================================================================

    /**
     * @brief Set a material property (uniform value)
     * @tparam T Type of the property (int, float, vec3, mat4, etc.)
     * @param name Uniform name in shader
     * @param value The value to set
     *
     * Type-safe property storage. Supported types: int, float, bool, vec2, vec3,
     * vec4, mat3, mat4. The value is cached and uploaded during bind().
     */
    template<typename T>
    void setProperty(const std::string& name, const T& value) {
        m_properties[name] = value;
    }

    /**
     * @brief Get a material property
     * @tparam T Expected type of the property
     * @param name Uniform name
     * @return Pointer to value if found and type matches, nullptr otherwise
     */
    template<typename T>
    [[nodiscard]] const T* getProperty(const std::string& name) const {
        auto it = m_properties.find(name);
        if (it != m_properties.end()) {
            return std::get_if<T>(&it->second);
        }
        return nullptr;
    }

    /**
     * @brief Remove a material property
     * @param name Uniform name
     * @return true if property was found and removed
     */
    bool removeProperty(const std::string& name);

    /**
     * @brief Get all material properties
     * @return Map of all properties (read-only)
     */
    [[nodiscard]] const std::unordered_map<std::string, UniformValue>& getProperties() const noexcept {
        return m_properties;
    }

    // ============================================================================
    // Shader Access
    // ============================================================================

    /**
     * @brief Get the shader program
     * @return Non-owning pointer to shader (can be nullptr)
     */
    [[nodiscard]] IShaderProgram* getShader() const noexcept { return m_shader; }

    /**
     * @brief Set a new shader program
     * @param shader Non-owning pointer to shader
     *
     * Note: Changing shader at runtime may require updating texture sampler names
     * and properties to match the new shader's uniforms.
     */
    void setShader(IShaderProgram* shader) noexcept { m_shader = shader; }

    // ============================================================================
    // Utility
    // ============================================================================

    /**
     * @brief Clear all textures and properties
     *
     * Removes all texture bindings and material properties. The shader reference
     * is preserved. Useful for resetting material state.
     */
    void clear();

protected:
    /**
     * @brief Upload a single uniform to the shader
     * @param name Uniform name
     * @param value Uniform value (variant type)
     *
     * Helper method that can be overridden in derived classes for custom
     * uniform handling. Uses visitor pattern with std::visit.
     */
    virtual void uploadUniform(const std::string& name, const UniformValue& value);

    /**
     * @brief Called before texture and property binding
     *
     * Hook for derived classes to perform custom pre-binding logic.
     */
    virtual void onPreBind() {}

    /**
     * @brief Called after texture and property binding
     *
     * Hook for derived classes to perform custom post-binding logic.
     */
    virtual void onPostBind() {}

private:
    /**
     * @brief Find next available texture unit
     * @return Next available unit number, or throws if all units in use
     * @throws std::runtime_error if all 32 texture units are occupied
     */
    unsigned int findAvailableTextureUnit() const;

    /**
     * @brief Check if a texture unit is already in use
     * @param unit Texture unit to check
     * @return true if unit is in use
     */
    bool isTextureUnitInUse(unsigned int unit) const;

    IShaderProgram* m_shader;                                      // Non-owning pointer
    std::vector<TextureBinding> m_textureBindings;                 // Ordered texture bindings
    std::unordered_map<std::string, UniformValue> m_properties;    // Material properties

    // Cache for sampler name -> texture binding index for O(1) lookup
    std::unordered_map<std::string, size_t> m_samplerNameToIndex;
};

// ================================================================================
// Specialized Material Types
// ================================================================================

/**
 * @brief Phong material with diffuse, specular, and normal mapping support
 *
 * Standard Phong lighting material with common properties pre-configured.
 * Expects shader with uniforms: u_DiffuseMap, u_SpecularMap, u_NormalMap,
 * u_Diffuse, u_Specular, u_Shininess, u_HasNormalMap, etc.
 */
class PhongMaterial : public Material {
public:
    explicit PhongMaterial(IShaderProgram* shader);

    // Convenience setters for common Phong properties
    void setDiffuseColor(const glm::vec3& color);
    void setSpecularColor(const glm::vec3& color);
    void setShininess(float shininess);

    // Texture setters with standard naming conventions
    void setDiffuseMap(std::shared_ptr<ITexture> texture);
    void setSpecularMap(std::shared_ptr<ITexture> texture);
    void setNormalMap(std::shared_ptr<ITexture> texture);

private:
    static constexpr unsigned int DIFFUSE_UNIT = 0;
    static constexpr unsigned int SPECULAR_UNIT = 1;
    static constexpr unsigned int NORMAL_UNIT = 2;
};

/**
 * @brief PBR (Physically Based Rendering) material
 *
 * Modern PBR material supporting metallic-roughness workflow.
 * Expects shader with uniforms: u_AlbedoMap, u_NormalMap, u_MetallicMap,
 * u_RoughnessMap, u_AOMap, u_Albedo, u_Metallic, u_Roughness, etc.
 */
class PBRMaterial : public Material {
public:
    explicit PBRMaterial(IShaderProgram* shader);

    // Convenience setters for PBR properties
    void setAlbedo(const glm::vec3& color);
    void setMetallic(float metallic);
    void setRoughness(float roughness);
    void setAO(float ao);

    // Texture setters with PBR naming conventions
    void setAlbedoMap(std::shared_ptr<ITexture> texture);
    void setNormalMap(std::shared_ptr<ITexture> texture);
    void setMetallicMap(std::shared_ptr<ITexture> texture);
    void setRoughnessMap(std::shared_ptr<ITexture> texture);
    void setAOMap(std::shared_ptr<ITexture> texture);
    void setMetallicRoughnessMap(std::shared_ptr<ITexture> texture); // Combined map

private:
    static constexpr unsigned int ALBEDO_UNIT = 0;
    static constexpr unsigned int NORMAL_UNIT = 1;
    static constexpr unsigned int METALLIC_UNIT = 2;
    static constexpr unsigned int ROUGHNESS_UNIT = 3;
    static constexpr unsigned int AO_UNIT = 4;
    static constexpr unsigned int METALLIC_ROUGHNESS_UNIT = 2; // Same as metallic for combined map
};

/**
 * @brief Material builder for fluent construction
 *
 * Optional builder pattern for more readable material setup.
 *
 * Usage:
 * @code
 *   auto material = MaterialBuilder(shader)
 *       .withTexture("u_DiffuseMap", diffuseTex, 0)
 *       .withProperty("u_Color", glm::vec3(1.0f))
 *       .withProperty("u_Shininess", 32.0f)
 *       .build();
 * @endcode
 */
class MaterialBuilder {
public:
    explicit MaterialBuilder(IShaderProgram* shader);

    MaterialBuilder& withTexture(const std::string& samplerName,
                                 std::shared_ptr<ITexture> texture,
                                 unsigned int unit);

    template<typename T>
    MaterialBuilder& withProperty(const std::string& name, const T& value) {
        m_material->setProperty(name, value);
        return *this;
    }

    [[nodiscard]] std::unique_ptr<Material> build();

private:
    std::unique_ptr<Material> m_material;
};

} // namespace Graphics
