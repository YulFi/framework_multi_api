#pragma once

#include <glad/glad.h>
#include <utility>

namespace OGL
{
    /**
     * RAII wrapper for OpenGL shader programs
     * Automatically deletes the program when the wrapper is destroyed
     * Internal helper class - use IShaderProgram for application code
     */
    class GLShaderProgram
    {
    public:
        GLShaderProgram() : m_id(0) {}

        explicit GLShaderProgram(GLuint id) : m_id(id) {}

        ~GLShaderProgram()
        {
            if (m_id != 0)
            {
                glDeleteProgram(m_id);
            }
        }

        // Delete copy constructor and assignment
        GLShaderProgram(const GLShaderProgram&) = delete;
        GLShaderProgram& operator=(const GLShaderProgram&) = delete;

        // Move constructor
        GLShaderProgram(GLShaderProgram&& other) noexcept : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        // Move assignment
        GLShaderProgram& operator=(GLShaderProgram&& other) noexcept
        {
            if (this != &other)
            {
                if (m_id != 0)
                {
                    glDeleteProgram(m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }

        GLuint get() const { return m_id; }
        GLuint release()
        {
            GLuint id = m_id;
            m_id = 0;
            return id;
        }

        bool isValid() const { return m_id != 0; }

    private:
        GLuint m_id;
    };

    /**
     * RAII wrapper for OpenGL shader objects (vertex/fragment shaders)
     * Internal helper class - use IShaderProgram for application code
     */
    class GLShader
    {
    public:
        GLShader() : m_id(0) {}

        explicit GLShader(GLuint id) : m_id(id) {}

        ~GLShader()
        {
            if (m_id != 0)
            {
                glDeleteShader(m_id);
            }
        }

        // Delete copy constructor and assignment
        GLShader(const GLShader&) = delete;
        GLShader& operator=(const GLShader&) = delete;

        // Move constructor
        GLShader(GLShader&& other) noexcept : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        // Move assignment
        GLShader& operator=(GLShader&& other) noexcept
        {
            if (this != &other)
            {
                if (m_id != 0)
                {
                    glDeleteShader(m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }

        GLuint get() const { return m_id; }
        GLuint release()
        {
            GLuint id = m_id;
            m_id = 0;
            return id;
        }

        bool isValid() const { return m_id != 0; }

    private:
        GLuint m_id;
    };

    /**
     * RAII wrapper for OpenGL buffer objects (VBO, IBO, etc.)
     */
    class Buffer
    {
    public:
        Buffer() : m_id(0) {}

        explicit Buffer(GLuint id) : m_id(id) {}

        ~Buffer()
        {
            if (m_id != 0)
            {
                glDeleteBuffers(1, &m_id);
            }
        }

        // Delete copy constructor and assignment
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        // Move constructor
        Buffer(Buffer&& other) noexcept : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        // Move assignment
        Buffer& operator=(Buffer&& other) noexcept
        {
            if (this != &other)
            {
                if (m_id != 0)
                {
                    glDeleteBuffers(1, &m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }

        GLuint get() const { return m_id; }
        GLuint release()
        {
            GLuint id = m_id;
            m_id = 0;
            return id;
        }

        bool isValid() const { return m_id != 0; }

    private:
        GLuint m_id;
    };

    /**
     * RAII wrapper for OpenGL vertex array objects
     */
    class VertexArrayObject
    {
    public:
        VertexArrayObject() : m_id(0) {}

        explicit VertexArrayObject(GLuint id) : m_id(id) {}

        ~VertexArrayObject()
        {
            if (m_id != 0)
            {
                glDeleteVertexArrays(1, &m_id);
            }
        }

        // Delete copy constructor and assignment
        VertexArrayObject(const VertexArrayObject&) = delete;
        VertexArrayObject& operator=(const VertexArrayObject&) = delete;

        // Move constructor
        VertexArrayObject(VertexArrayObject&& other) noexcept : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        // Move assignment
        VertexArrayObject& operator=(VertexArrayObject&& other) noexcept
        {
            if (this != &other)
            {
                if (m_id != 0)
                {
                    glDeleteVertexArrays(1, &m_id);
                }
                m_id = other.m_id;
                other.m_id = 0;
            }
            return *this;
        }

        GLuint get() const { return m_id; }
        GLuint release()
        {
            GLuint id = m_id;
            m_id = 0;
            return id;
        }

        bool isValid() const { return m_id != 0; }

    private:
        GLuint m_id;
    };

} // namespace OGL
