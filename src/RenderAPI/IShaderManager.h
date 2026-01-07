#pragma once

#include <glm/glm.hpp>
#include <string>

class IShaderManager
{
public:
    virtual ~IShaderManager() = default;

    virtual bool loadShaderFromFile(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) = 0;
    virtual void use(const std::string& name) = 0;
    virtual void unuse() = 0;

    virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
    virtual void setVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;

    virtual void cleanup() = 0;
};
