#pragma once

#include <glm/glm.hpp>
#include <string>

class IShaderProgram;  // Forward declaration

/**
 * Shader Manager Interface
 * Responsible for loading, storing, and cleaning up shader programs
 * Returns shader wrappers that can be used directly in application code
 */
class IShaderManager
{
public:
    virtual ~IShaderManager() = default;

    /**
     * Create and load a shader program from vertex and fragment shader files
     * Returns a non-owning pointer to the shader program (ShaderManager retains ownership)
     * @param name Unique name for this shader
     * @param vertexPath Path to vertex shader file (relative to shader base path)
     * @param fragmentPath Path to fragment shader file (relative to shader base path)
     * @return Non-owning pointer to shader program, or nullptr on failure
     */
    virtual IShaderProgram* createShaderProgram(
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath) = 0;

    /**
     * Get an existing shader program by name
     * @param name Name of the shader to retrieve
     * @return Non-owning pointer to shader program, or nullptr if not found
     */
    virtual IShaderProgram* getShader(const std::string& name) = 0;

    /**
     * Clean up all shader resources
     */
    virtual void cleanup() = 0;
};
