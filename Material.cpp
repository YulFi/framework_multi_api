#include "Material.h"
#include "src/RenderAPI/IShaderProgram.h"
#include "src/RenderAPI/ITexture.h"
#include "src/Logger.h"
#include <algorithm>
#include <stdexcept>

namespace Graphics {

// ================================================================================
// Material Base Class Implementation
// ================================================================================

Material::Material(IShaderProgram* shader)
    : m_shader(shader) {
    if (!shader) {
        throw std::invalid_argument("Material: shader cannot be nullptr");
    }
}

void Material::bind() {
    static bool firstBind = true;

    if (!m_shader) {
        throw std::runtime_error("Material::bind() - shader is null");
    }

    // Pre-binding hook for derived classes
    onPreBind();

    // Bind shader program
    m_shader->bind();

    if (firstBind) {
        LOG_INFO("Material::bind() - First bind");
        LOG_INFO("  Texture bindings: {}", m_textureBindings.size());
        LOG_INFO("  Properties (uniforms): {}", m_properties.size());
        firstBind = false;
    }

    // Bind all textures and set sampler uniforms
    for (const auto& binding : m_textureBindings) {
        if (binding.texture) {
            // Bind texture to its designated unit
            binding.texture->bind(binding.textureUnit);

            // Set the sampler uniform to the texture unit
            m_shader->setInt(binding.samplerName, static_cast<int>(binding.textureUnit));
        }
    }

    // Upload all material properties as uniforms
    for (const auto& [name, value] : m_properties) {
        uploadUniform(name, value);
    }

    // Post-binding hook for derived classes
    onPostBind();
}

void Material::unbind() {
    // Unbind textures in reverse order
    for (auto it = m_textureBindings.rbegin(); it != m_textureBindings.rend(); ++it) {
        if (it->texture) {
            it->texture->unbind();
        }
    }

    // Unbind shader
    if (m_shader) {
        m_shader->unbind();
    }
}

void Material::setTexture(const std::string& samplerName,
                         std::shared_ptr<ITexture> texture,
                         unsigned int textureUnit) {
    // Check if sampler name already exists
    auto nameIt = m_samplerNameToIndex.find(samplerName);
    if (nameIt != m_samplerNameToIndex.end()) {
        // Update existing binding
        size_t index = nameIt->second;
        unsigned int oldUnit = m_textureBindings[index].textureUnit;

        // If changing unit, check new unit isn't in use by another texture
        if (oldUnit != textureUnit && isTextureUnitInUse(textureUnit)) {
            throw std::invalid_argument("Material::setTexture() - texture unit " +
                                      std::to_string(textureUnit) + " is already in use");
        }

        m_textureBindings[index].texture = std::move(texture);
        m_textureBindings[index].textureUnit = textureUnit;
        return;
    }

    // Check if texture unit is already in use
    if (isTextureUnitInUse(textureUnit)) {
        throw std::invalid_argument("Material::setTexture() - texture unit " +
                                  std::to_string(textureUnit) + " is already in use");
    }

    // Add new binding
    size_t index = m_textureBindings.size();
    m_textureBindings.emplace_back(std::move(texture), samplerName, textureUnit);
    m_samplerNameToIndex[samplerName] = index;
}

unsigned int Material::setTexture(const std::string& samplerName,
                                 std::shared_ptr<ITexture> texture) {
    // Check if sampler name already exists
    auto nameIt = m_samplerNameToIndex.find(samplerName);
    if (nameIt != m_samplerNameToIndex.end()) {
        // Reuse existing unit
        size_t index = nameIt->second;
        unsigned int unit = m_textureBindings[index].textureUnit;
        m_textureBindings[index].texture = std::move(texture);
        return unit;
    }

    // Find next available unit
    unsigned int unit = findAvailableTextureUnit();

    // Add new binding
    size_t index = m_textureBindings.size();
    m_textureBindings.emplace_back(std::move(texture), samplerName, unit);
    m_samplerNameToIndex[samplerName] = index;

    return unit;
}

bool Material::removeTexture(const std::string& samplerName) {
    auto it = m_samplerNameToIndex.find(samplerName);
    if (it == m_samplerNameToIndex.end()) {
        return false;
    }

    size_t removeIndex = it->second;

    // Remove from bindings vector (swap with last element for O(1) removal)
    if (removeIndex < m_textureBindings.size() - 1) {
        // Update the index of the swapped element
        const std::string& swappedName = m_textureBindings.back().samplerName;
        m_samplerNameToIndex[swappedName] = removeIndex;

        // Swap and pop
        std::swap(m_textureBindings[removeIndex], m_textureBindings.back());
    }
    m_textureBindings.pop_back();

    // Remove from name map
    m_samplerNameToIndex.erase(it);

    return true;
}

std::shared_ptr<ITexture> Material::getTexture(const std::string& samplerName) const {
    auto it = m_samplerNameToIndex.find(samplerName);
    if (it != m_samplerNameToIndex.end()) {
        return m_textureBindings[it->second].texture;
    }
    return nullptr;
}

bool Material::removeProperty(const std::string& name) {
    return m_properties.erase(name) > 0;
}

void Material::clear() {
    m_textureBindings.clear();
    m_samplerNameToIndex.clear();
    m_properties.clear();
}

void Material::uploadUniform(const std::string& name, const UniformValue& value) {
    if (!m_shader) {
        return;
    }

    // Use std::visit to handle variant types
    std::visit([this, &name](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int>) {
            m_shader->setInt(name, arg);
        }
        else if constexpr (std::is_same_v<T, float>) {
            m_shader->setFloat(name, arg);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            m_shader->setInt(name, arg ? 1 : 0);
        }
        else if constexpr (std::is_same_v<T, glm::vec2>) {
            m_shader->setVec2(name, arg);
        }
        else if constexpr (std::is_same_v<T, glm::vec3>) {
            m_shader->setVec3(name, arg);
        }
        else if constexpr (std::is_same_v<T, glm::vec4>) {
            m_shader->setVec4(name, arg);
        }
        else if constexpr (std::is_same_v<T, glm::mat3>) {
            m_shader->setMat3(name, arg);
        }
        else if constexpr (std::is_same_v<T, glm::mat4>) {
            m_shader->setMat4(name, arg);
        }
    }, value);
}

unsigned int Material::findAvailableTextureUnit() const {
    // Start from 0 and find first available unit
    // Maximum texture units is typically 32 for modern GPUs
    constexpr unsigned int MAX_TEXTURE_UNITS = 32;

    for (unsigned int unit = 0; unit < MAX_TEXTURE_UNITS; ++unit) {
        if (!isTextureUnitInUse(unit)) {
            return unit;
        }
    }

    throw std::runtime_error("Material::findAvailableTextureUnit() - all texture units in use");
}

bool Material::isTextureUnitInUse(unsigned int unit) const {
    return std::any_of(m_textureBindings.begin(), m_textureBindings.end(),
                      [unit](const TextureBinding& binding) {
                          return binding.textureUnit == unit;
                      });
}

// ================================================================================
// PhongMaterial Implementation
// ================================================================================

PhongMaterial::PhongMaterial(IShaderProgram* shader)
    : Material(shader) {
    // Initialize default Phong properties
    setDiffuseColor(glm::vec3(1.0f, 1.0f, 1.0f));
    setSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));
    setShininess(32.0f);

    // Set flags for texture usage (updated when textures are set)
    setProperty("u_HasDiffuseMap", false);
    setProperty("u_HasSpecularMap", false);
    setProperty("u_HasNormalMap", false);
}

void PhongMaterial::setDiffuseColor(const glm::vec3& color) {
    setProperty("u_Diffuse", color);
}

void PhongMaterial::setSpecularColor(const glm::vec3& color) {
    setProperty("u_Specular", color);
}

void PhongMaterial::setShininess(float shininess) {
    setProperty("u_Shininess", shininess);
}

void PhongMaterial::setDiffuseMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_DiffuseMap", texture, DIFFUSE_UNIT);
        setProperty("u_HasDiffuseMap", true);
    } else {
        removeTexture("u_DiffuseMap");
        setProperty("u_HasDiffuseMap", false);
    }
}

void PhongMaterial::setSpecularMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_SpecularMap", texture, SPECULAR_UNIT);
        setProperty("u_HasSpecularMap", true);
    } else {
        removeTexture("u_SpecularMap");
        setProperty("u_HasSpecularMap", false);
    }
}

void PhongMaterial::setNormalMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_NormalMap", texture, NORMAL_UNIT);
        setProperty("u_HasNormalMap", true);
    } else {
        removeTexture("u_NormalMap");
        setProperty("u_HasNormalMap", false);
    }
}

// ================================================================================
// PBRMaterial Implementation
// ================================================================================

PBRMaterial::PBRMaterial(IShaderProgram* shader)
    : Material(shader) {
    // Initialize default PBR properties
    setAlbedo(glm::vec3(1.0f, 1.0f, 1.0f));
    setMetallic(0.0f);
    setRoughness(0.5f);
    setAO(1.0f);

    // Set flags for texture usage
    setProperty("u_HasAlbedoMap", false);
    setProperty("u_HasNormalMap", false);
    setProperty("u_HasMetallicMap", false);
    setProperty("u_HasRoughnessMap", false);
    setProperty("u_HasAOMap", false);
    setProperty("u_HasMetallicRoughnessMap", false);
}

void PBRMaterial::setAlbedo(const glm::vec3& color) {
    setProperty("u_Albedo", color);
}

void PBRMaterial::setMetallic(float metallic) {
    setProperty("u_Metallic", metallic);
}

void PBRMaterial::setRoughness(float roughness) {
    setProperty("u_Roughness", roughness);
}

void PBRMaterial::setAO(float ao) {
    setProperty("u_AO", ao);
}

void PBRMaterial::setAlbedoMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_AlbedoMap", texture, ALBEDO_UNIT);
        setProperty("u_HasAlbedoMap", true);
    } else {
        removeTexture("u_AlbedoMap");
        setProperty("u_HasAlbedoMap", false);
    }
}

void PBRMaterial::setNormalMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_NormalMap", texture, NORMAL_UNIT);
        setProperty("u_HasNormalMap", true);
    } else {
        removeTexture("u_NormalMap");
        setProperty("u_HasNormalMap", false);
    }
}

void PBRMaterial::setMetallicMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_MetallicMap", texture, METALLIC_UNIT);
        setProperty("u_HasMetallicMap", true);
        setProperty("u_HasMetallicRoughnessMap", false); // Disable combined map
    } else {
        removeTexture("u_MetallicMap");
        setProperty("u_HasMetallicMap", false);
    }
}

void PBRMaterial::setRoughnessMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_RoughnessMap", texture, ROUGHNESS_UNIT);
        setProperty("u_HasRoughnessMap", true);
        setProperty("u_HasMetallicRoughnessMap", false); // Disable combined map
    } else {
        removeTexture("u_RoughnessMap");
        setProperty("u_HasRoughnessMap", false);
    }
}

void PBRMaterial::setAOMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        setTexture("u_AOMap", texture, AO_UNIT);
        setProperty("u_HasAOMap", true);
    } else {
        removeTexture("u_AOMap");
        setProperty("u_HasAOMap", false);
    }
}

void PBRMaterial::setMetallicRoughnessMap(std::shared_ptr<ITexture> texture) {
    if (texture) {
        // Combined map uses metallic unit, roughness is removed
        setTexture("u_MetallicRoughnessMap", texture, METALLIC_ROUGHNESS_UNIT);
        setProperty("u_HasMetallicRoughnessMap", true);
        setProperty("u_HasMetallicMap", false);
        setProperty("u_HasRoughnessMap", false);

        // Clean up separate maps if they exist
        removeTexture("u_MetallicMap");
        removeTexture("u_RoughnessMap");
    } else {
        removeTexture("u_MetallicRoughnessMap");
        setProperty("u_HasMetallicRoughnessMap", false);
    }
}

// ================================================================================
// MaterialBuilder Implementation
// ================================================================================

MaterialBuilder::MaterialBuilder(IShaderProgram* shader)
    : m_material(std::make_unique<Material>(shader)) {
}

MaterialBuilder& MaterialBuilder::withTexture(const std::string& samplerName,
                                             std::shared_ptr<ITexture> texture,
                                             unsigned int unit) {
    m_material->setTexture(samplerName, std::move(texture), unit);
    return *this;
}

std::unique_ptr<Material> MaterialBuilder::build() {
    return std::move(m_material);
}

} // namespace Graphics
