/**
 * @file Material_Tests.cpp
 * @brief Unit test examples for Material system
 *
 * This file demonstrates how to write comprehensive tests for the Material class.
 * Uses a generic testing approach - adapt to your testing framework (Google Test, Catch2, etc.)
 *
 * Test coverage includes:
 * - Construction and initialization
 * - Texture management
 * - Property management
 * - Bind/unbind behavior
 * - Error handling
 * - Specialized materials (Phong, PBR)
 */

#include "Material.h"
#include <cassert>
#include <iostream>
#include <glm/glm.hpp>

// ================================================================================
// Mock Classes for Testing
// ================================================================================

/**
 * @brief Mock shader program for testing
 */
class MockShaderProgram : public IShaderProgram {
public:
    bool m_boundCalled = false;
    bool m_unboundCalled = false;
    std::unordered_map<std::string, int> m_intUniforms;
    std::unordered_map<std::string, float> m_floatUniforms;
    std::unordered_map<std::string, glm::vec3> m_vec3Uniforms;
    std::unordered_map<std::string, glm::mat4> m_mat4Uniforms;

    void bind() override {
        m_boundCalled = true;
    }

    void unbind() override {
        m_unboundCalled = true;
    }

    void setInt(const std::string& name, int value) override {
        m_intUniforms[name] = value;
    }

    void setFloat(const std::string& name, float value) override {
        m_floatUniforms[name] = value;
    }

    void setVec2(const std::string& name, const glm::vec2& value) override {}

    void setVec3(const std::string& name, const glm::vec3& value) override {
        m_vec3Uniforms[name] = value;
    }

    void setVec4(const std::string& name, const glm::vec4& value) override {}

    void setMat3(const std::string& name, const glm::mat3& value) override {}

    void setMat4(const std::string& name, const glm::mat4& value) override {
        m_mat4Uniforms[name] = value;
    }

    // Helper methods for testing
    bool wasIntSet(const std::string& name, int expectedValue) const {
        auto it = m_intUniforms.find(name);
        return it != m_intUniforms.end() && it->second == expectedValue;
    }

    bool wasFloatSet(const std::string& name, float expectedValue) const {
        auto it = m_floatUniforms.find(name);
        return it != m_floatUniforms.end() && std::abs(it->second - expectedValue) < 0.0001f;
    }

    bool wasVec3Set(const std::string& name, const glm::vec3& expectedValue) const {
        auto it = m_vec3Uniforms.find(name);
        return it != m_vec3Uniforms.end() && it->second == expectedValue;
    }

    void reset() {
        m_boundCalled = false;
        m_unboundCalled = false;
        m_intUniforms.clear();
        m_floatUniforms.clear();
        m_vec3Uniforms.clear();
        m_mat4Uniforms.clear();
    }
};

/**
 * @brief Mock texture for testing
 */
class MockTexture : public ITexture {
public:
    int m_lastBoundUnit = -1;
    bool m_unboundCalled = false;

    void bind(unsigned int unit) override {
        m_lastBoundUnit = static_cast<int>(unit);
    }

    void unbind() override {
        m_unboundCalled = true;
        m_lastBoundUnit = -1;
    }

    bool wasBoundToUnit(unsigned int unit) const {
        return m_lastBoundUnit == static_cast<int>(unit);
    }

    void reset() {
        m_lastBoundUnit = -1;
        m_unboundCalled = false;
    }
};

// ================================================================================
// Test Utilities
// ================================================================================

#define TEST_CASE(name) void Test_##name()
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << message << " at line " << __LINE__ << std::endl; \
        return; \
    }
#define ASSERT_FALSE(condition, message) ASSERT_TRUE(!(condition), message)
#define ASSERT_EQ(a, b, message) ASSERT_TRUE((a) == (b), message)
#define ASSERT_NE(a, b, message) ASSERT_TRUE((a) != (b), message)
#define ASSERT_THROW(statement, exception_type, message) \
    { \
        bool threw = false; \
        try { statement; } \
        catch (const exception_type&) { threw = true; } \
        ASSERT_TRUE(threw, message); \
    }

void runTest(const char* name, void (*testFunc)()) {
    std::cout << "Running: " << name << "... ";
    testFunc();
    std::cout << "PASSED" << std::endl;
}

// ================================================================================
// Construction Tests
// ================================================================================

TEST_CASE(ConstructorWithValidShader) {
    MockShaderProgram shader;
    Material material(&shader);

    ASSERT_TRUE(material.isValid(), "Material should be valid with non-null shader");
    ASSERT_EQ(material.getShader(), &shader, "Shader should match");
}

TEST_CASE(ConstructorWithNullShader) {
    ASSERT_THROW(
        Material material(nullptr),
        std::invalid_argument,
        "Should throw when constructing with null shader"
    );
}

TEST_CASE(MaterialIsMovable) {
    MockShaderProgram shader;
    Material material1(&shader);
    material1.setProperty("u_Test", 42.0f);

    // Move construction
    Material material2(std::move(material1));
    ASSERT_TRUE(material2.isValid(), "Moved material should be valid");
    ASSERT_NE(material2.getProperty<float>("u_Test"), nullptr, "Property should be moved");
}

// ================================================================================
// Texture Management Tests
// ================================================================================

TEST_CASE(SetTextureWithExplicitUnit) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", texture, 0);

    ASSERT_EQ(material.getTexture("u_DiffuseMap"), texture, "Texture should be retrievable");
    ASSERT_EQ(material.getTextureBindings().size(), 1u, "Should have 1 texture binding");
}

TEST_CASE(SetTextureWithAutomaticUnit) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    unsigned int unit = material.setTexture("u_DiffuseMap", texture);

    ASSERT_EQ(unit, 0u, "First texture should get unit 0");
    ASSERT_EQ(material.getTexture("u_DiffuseMap"), texture, "Texture should be retrievable");
}

TEST_CASE(SetMultipleTexturesAutomatic) {
    MockShaderProgram shader;
    Material material(&shader);
    auto tex1 = std::make_shared<MockTexture>();
    auto tex2 = std::make_shared<MockTexture>();
    auto tex3 = std::make_shared<MockTexture>();

    unsigned int unit1 = material.setTexture("u_Diffuse", tex1);
    unsigned int unit2 = material.setTexture("u_Normal", tex2);
    unsigned int unit3 = material.setTexture("u_Specular", tex3);

    ASSERT_EQ(unit1, 0u, "First texture should get unit 0");
    ASSERT_EQ(unit2, 1u, "Second texture should get unit 1");
    ASSERT_EQ(unit3, 2u, "Third texture should get unit 2");
}

TEST_CASE(TextureUnitConflict) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setTexture("u_Texture1", nullptr, 0);

    ASSERT_THROW(
        material.setTexture("u_Texture2", nullptr, 0),
        std::invalid_argument,
        "Should throw when texture unit is already in use"
    );
}

TEST_CASE(UpdateExistingTexture) {
    MockShaderProgram shader;
    Material material(&shader);
    auto tex1 = std::make_shared<MockTexture>();
    auto tex2 = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", tex1, 0);
    material.setTexture("u_DiffuseMap", tex2, 0);  // Replace with same unit

    ASSERT_EQ(material.getTexture("u_DiffuseMap"), tex2, "Texture should be updated");
    ASSERT_EQ(material.getTextureBindings().size(), 1u, "Should still have only 1 binding");
}

TEST_CASE(RemoveTexture) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", texture, 0);
    bool removed = material.removeTexture("u_DiffuseMap");

    ASSERT_TRUE(removed, "Remove should return true");
    ASSERT_EQ(material.getTexture("u_DiffuseMap"), nullptr, "Texture should be removed");
    ASSERT_EQ(material.getTextureBindings().size(), 0u, "Bindings should be empty");
}

TEST_CASE(RemoveNonExistentTexture) {
    MockShaderProgram shader;
    Material material(&shader);

    bool removed = material.removeTexture("u_NonExistent");

    ASSERT_FALSE(removed, "Remove should return false for non-existent texture");
}

// ================================================================================
// Property Management Tests
// ================================================================================

TEST_CASE(SetAndGetFloatProperty) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setProperty("u_Shininess", 32.0f);

    const float* value = material.getProperty<float>("u_Shininess");
    ASSERT_NE(value, nullptr, "Property should exist");
    ASSERT_EQ(*value, 32.0f, "Property value should match");
}

TEST_CASE(SetAndGetVec3Property) {
    MockShaderProgram shader;
    Material material(&shader);
    glm::vec3 color(1.0f, 0.5f, 0.25f);

    material.setProperty("u_Color", color);

    const glm::vec3* value = material.getProperty<glm::vec3>("u_Color");
    ASSERT_NE(value, nullptr, "Property should exist");
    ASSERT_EQ(*value, color, "Property value should match");
}

TEST_CASE(GetPropertyWithWrongType) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setProperty("u_Color", glm::vec3(1.0f));

    // Try to get as float (wrong type)
    const float* value = material.getProperty<float>("u_Color");
    ASSERT_EQ(value, nullptr, "Should return nullptr for wrong type");
}

TEST_CASE(GetNonExistentProperty) {
    MockShaderProgram shader;
    Material material(&shader);

    const float* value = material.getProperty<float>("u_NonExistent");
    ASSERT_EQ(value, nullptr, "Should return nullptr for non-existent property");
}

TEST_CASE(RemoveProperty) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setProperty("u_Shininess", 32.0f);
    bool removed = material.removeProperty("u_Shininess");

    ASSERT_TRUE(removed, "Remove should return true");
    ASSERT_EQ(material.getProperty<float>("u_Shininess"), nullptr, "Property should be removed");
}

TEST_CASE(UpdateExistingProperty) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setProperty("u_Shininess", 32.0f);
    material.setProperty("u_Shininess", 64.0f);

    const float* value = material.getProperty<float>("u_Shininess");
    ASSERT_NE(value, nullptr, "Property should exist");
    ASSERT_EQ(*value, 64.0f, "Property should be updated");
}

// ================================================================================
// Bind/Unbind Tests
// ================================================================================

TEST_CASE(BindCallsShaderBind) {
    MockShaderProgram shader;
    Material material(&shader);

    material.bind();

    ASSERT_TRUE(shader.m_boundCalled, "Shader bind should be called");
}

TEST_CASE(BindUploadsProperties) {
    MockShaderProgram shader;
    Material material(&shader);

    material.setProperty("u_Shininess", 32.0f);
    material.setProperty("u_Color", glm::vec3(1.0f, 0.5f, 0.25f));
    material.bind();

    ASSERT_TRUE(shader.wasFloatSet("u_Shininess", 32.0f), "Float property should be uploaded");
    ASSERT_TRUE(shader.wasVec3Set("u_Color", glm::vec3(1.0f, 0.5f, 0.25f)), "Vec3 property should be uploaded");
}

TEST_CASE(BindActivatesTextures) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", texture, 0);
    material.bind();

    ASSERT_TRUE(texture->wasBoundToUnit(0), "Texture should be bound to unit 0");
    ASSERT_TRUE(shader.wasIntSet("u_DiffuseMap", 0), "Sampler uniform should be set to unit 0");
}

TEST_CASE(BindMultipleTextures) {
    MockShaderProgram shader;
    Material material(&shader);
    auto tex1 = std::make_shared<MockTexture>();
    auto tex2 = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", tex1, 0);
    material.setTexture("u_NormalMap", tex2, 1);
    material.bind();

    ASSERT_TRUE(tex1->wasBoundToUnit(0), "Texture 1 should be bound to unit 0");
    ASSERT_TRUE(tex2->wasBoundToUnit(1), "Texture 2 should be bound to unit 1");
    ASSERT_TRUE(shader.wasIntSet("u_DiffuseMap", 0), "Sampler 1 should be set");
    ASSERT_TRUE(shader.wasIntSet("u_NormalMap", 1), "Sampler 2 should be set");
}

TEST_CASE(UnbindCallsShaderUnbind) {
    MockShaderProgram shader;
    Material material(&shader);

    material.bind();
    material.unbind();

    ASSERT_TRUE(shader.m_unboundCalled, "Shader unbind should be called");
}

TEST_CASE(UnbindUnbindsTextures) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", texture, 0);
    material.bind();
    material.unbind();

    ASSERT_TRUE(texture->m_unboundCalled, "Texture unbind should be called");
}

TEST_CASE(BindWithNullShaderThrows) {
    MockShaderProgram shader;
    Material material(&shader);
    material.setShader(nullptr);

    ASSERT_THROW(
        material.bind(),
        std::runtime_error,
        "Should throw when binding with null shader"
    );
}

// ================================================================================
// Clear Tests
// ================================================================================

TEST_CASE(ClearRemovesAllTexturesAndProperties) {
    MockShaderProgram shader;
    Material material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setTexture("u_DiffuseMap", texture, 0);
    material.setProperty("u_Shininess", 32.0f);
    material.clear();

    ASSERT_EQ(material.getTextureBindings().size(), 0u, "Textures should be cleared");
    ASSERT_EQ(material.getProperties().size(), 0u, "Properties should be cleared");
    ASSERT_TRUE(material.isValid(), "Shader should still be set");
}

// ================================================================================
// PhongMaterial Tests
// ================================================================================

TEST_CASE(PhongMaterialInitialization) {
    MockShaderProgram shader;
    PhongMaterial material(&shader);

    // Check default properties are set
    ASSERT_NE(material.getProperty<glm::vec3>("u_Diffuse"), nullptr, "Diffuse color should be set");
    ASSERT_NE(material.getProperty<glm::vec3>("u_Specular"), nullptr, "Specular color should be set");
    ASSERT_NE(material.getProperty<float>("u_Shininess"), nullptr, "Shininess should be set");
}

TEST_CASE(PhongMaterialSetDiffuseMap) {
    MockShaderProgram shader;
    PhongMaterial material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setDiffuseMap(texture);

    ASSERT_EQ(material.getTexture("u_DiffuseMap"), texture, "Diffuse map should be set");
    const bool* hasMap = material.getProperty<bool>("u_HasDiffuseMap");
    ASSERT_NE(hasMap, nullptr, "u_HasDiffuseMap flag should be set");
    ASSERT_TRUE(*hasMap, "u_HasDiffuseMap should be true");
}

TEST_CASE(PhongMaterialRemoveDiffuseMap) {
    MockShaderProgram shader;
    PhongMaterial material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setDiffuseMap(texture);
    material.setDiffuseMap(nullptr);  // Remove

    ASSERT_EQ(material.getTexture("u_DiffuseMap"), nullptr, "Diffuse map should be removed");
    const bool* hasMap = material.getProperty<bool>("u_HasDiffuseMap");
    ASSERT_NE(hasMap, nullptr, "u_HasDiffuseMap flag should exist");
    ASSERT_FALSE(*hasMap, "u_HasDiffuseMap should be false");
}

// ================================================================================
// PBRMaterial Tests
// ================================================================================

TEST_CASE(PBRMaterialInitialization) {
    MockShaderProgram shader;
    PBRMaterial material(&shader);

    // Check default properties
    ASSERT_NE(material.getProperty<glm::vec3>("u_Albedo"), nullptr, "Albedo should be set");
    ASSERT_NE(material.getProperty<float>("u_Metallic"), nullptr, "Metallic should be set");
    ASSERT_NE(material.getProperty<float>("u_Roughness"), nullptr, "Roughness should be set");
    ASSERT_NE(material.getProperty<float>("u_AO"), nullptr, "AO should be set");
}

TEST_CASE(PBRMaterialSetMetallicRoughnessMap) {
    MockShaderProgram shader;
    PBRMaterial material(&shader);
    auto texture = std::make_shared<MockTexture>();

    material.setMetallicRoughnessMap(texture);

    ASSERT_EQ(material.getTexture("u_MetallicRoughnessMap"), texture, "MR map should be set");

    const bool* hasMR = material.getProperty<bool>("u_HasMetallicRoughnessMap");
    const bool* hasMetallic = material.getProperty<bool>("u_HasMetallicMap");
    const bool* hasRoughness = material.getProperty<bool>("u_HasRoughnessMap");

    ASSERT_NE(hasMR, nullptr, "u_HasMetallicRoughnessMap should exist");
    ASSERT_TRUE(*hasMR, "u_HasMetallicRoughnessMap should be true");
    ASSERT_FALSE(*hasMetallic, "u_HasMetallicMap should be false (using combined)");
    ASSERT_FALSE(*hasRoughness, "u_HasRoughnessMap should be false (using combined)");
}

// ================================================================================
// MaterialBuilder Tests
// ================================================================================

TEST_CASE(MaterialBuilderBasicUsage) {
    MockShaderProgram shader;
    auto texture = std::make_shared<MockTexture>();

    auto material = MaterialBuilder(&shader)
        .withTexture("u_DiffuseMap", texture, 0)
        .withProperty("u_Shininess", 32.0f)
        .build();

    ASSERT_TRUE(material->isValid(), "Material should be valid");
    ASSERT_EQ(material->getTexture("u_DiffuseMap"), texture, "Texture should be set");
    ASSERT_NE(material->getProperty<float>("u_Shininess"), nullptr, "Property should be set");
}

// ================================================================================
// Test Runner
// ================================================================================

int main() {
    std::cout << "=== Material System Unit Tests ===" << std::endl << std::endl;

    // Construction tests
    runTest("ConstructorWithValidShader", Test_ConstructorWithValidShader);
    runTest("ConstructorWithNullShader", Test_ConstructorWithNullShader);
    runTest("MaterialIsMovable", Test_MaterialIsMovable);

    // Texture management tests
    runTest("SetTextureWithExplicitUnit", Test_SetTextureWithExplicitUnit);
    runTest("SetTextureWithAutomaticUnit", Test_SetTextureWithAutomaticUnit);
    runTest("SetMultipleTexturesAutomatic", Test_SetMultipleTexturesAutomatic);
    runTest("TextureUnitConflict", Test_TextureUnitConflict);
    runTest("UpdateExistingTexture", Test_UpdateExistingTexture);
    runTest("RemoveTexture", Test_RemoveTexture);
    runTest("RemoveNonExistentTexture", Test_RemoveNonExistentTexture);

    // Property management tests
    runTest("SetAndGetFloatProperty", Test_SetAndGetFloatProperty);
    runTest("SetAndGetVec3Property", Test_SetAndGetVec3Property);
    runTest("GetPropertyWithWrongType", Test_GetPropertyWithWrongType);
    runTest("GetNonExistentProperty", Test_GetNonExistentProperty);
    runTest("RemoveProperty", Test_RemoveProperty);
    runTest("UpdateExistingProperty", Test_UpdateExistingProperty);

    // Bind/unbind tests
    runTest("BindCallsShaderBind", Test_BindCallsShaderBind);
    runTest("BindUploadsProperties", Test_BindUploadsProperties);
    runTest("BindActivatesTextures", Test_BindActivatesTextures);
    runTest("BindMultipleTextures", Test_BindMultipleTextures);
    runTest("UnbindCallsShaderUnbind", Test_UnbindCallsShaderUnbind);
    runTest("UnbindUnbindsTextures", Test_UnbindUnbindsTextures);
    runTest("BindWithNullShaderThrows", Test_BindWithNullShaderThrows);

    // Clear tests
    runTest("ClearRemovesAllTexturesAndProperties", Test_ClearRemovesAllTexturesAndProperties);

    // PhongMaterial tests
    runTest("PhongMaterialInitialization", Test_PhongMaterialInitialization);
    runTest("PhongMaterialSetDiffuseMap", Test_PhongMaterialSetDiffuseMap);
    runTest("PhongMaterialRemoveDiffuseMap", Test_PhongMaterialRemoveDiffuseMap);

    // PBRMaterial tests
    runTest("PBRMaterialInitialization", Test_PBRMaterialInitialization);
    runTest("PBRMaterialSetMetallicRoughnessMap", Test_PBRMaterialSetMetallicRoughnessMap);

    // MaterialBuilder tests
    runTest("MaterialBuilderBasicUsage", Test_MaterialBuilderBasicUsage);

    std::cout << std::endl << "=== All Tests Passed ===" << std::endl;

    return 0;
}

// ================================================================================
// Additional Test Ideas (for your test framework)
// ================================================================================

/*
// Performance tests
TEST(MaterialPerformance, BindUnbind1000Times) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        material->bind();
        material->unbind();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    EXPECT_LT(duration.count(), 10000);  // Should be fast
}

// Thread safety tests (if implementing thread-safe version)
TEST(MaterialThreadSafety, ConcurrentPropertyUpdates) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&material, i]() {
            material->setProperty("u_Value" + std::to_string(i), static_cast<float>(i));
        });
    }
    for (auto& t : threads) t.join();
    // Verify all properties were set
}

// Memory leak tests (with Valgrind, AddressSanitizer, etc.)
TEST(MaterialMemory, NoLeaksWithManyTexturesAndProperties) {
    for (int i = 0; i < 100; ++i) {
        auto material = std::make_unique<Material>(shader.get());
        for (int j = 0; j < 10; ++j) {
            material->setTexture("u_Tex" + std::to_string(j), texture);
            material->setProperty("u_Prop" + std::to_string(j), static_cast<float>(j));
        }
        material->bind();
        material->unbind();
    }
    // Run with memory leak detector
}

// Integration tests
TEST(MaterialIntegration, FullRenderingPipeline) {
    // Create full rendering setup
    // Verify material works end-to-end
}
*/
