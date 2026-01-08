#pragma once

#include <glm/glm.hpp>
#include <string>

/**
 * Abstract interface for shader programs
 * Platform-agnostic shader wrapper that allows direct shader manipulation
 * Follows the same pattern as IVertexArray/IVertexBuffer
 */
class IShaderProgram
{
public:
    virtual ~IShaderProgram() = default;

    /**
     * Bind this shader program for rendering
     * OpenGL: calls glUseProgram()
     * Vulkan: sets as current shader for next draw call
     */
    virtual void bind() = 0;

    /**
     * Unbind this shader program
     */
    virtual void unbind() = 0;

    // Uniform setters
    virtual void setBool(const std::string& name, bool value) = 0;
    virtual void setInt(const std::string& name, int value) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;
    virtual void setVec2(const std::string& name, const glm::vec2& value) = 0;
    virtual void setVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void setVec4(const std::string& name, const glm::vec4& value) = 0;
    virtual void setMat3(const std::string& name, const glm::mat3& value) = 0;
    virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;

    /**
     * Check if the shader program is valid and can be used
     */
    virtual bool isValid() const = 0;

    /**
     * Get the name of this shader program
     */
    virtual const std::string& getName() const = 0;
};
