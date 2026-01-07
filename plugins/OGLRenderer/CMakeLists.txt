cmake_minimum_required(VERSION 3.15)
project(OGLRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Create shared library
add_library(${PROJECT_NAME} SHARED
    OGLPlugin.cpp
    ../../src/OGL/Renderer.cpp
    ../../src/OGL/ShaderManager.cpp
    ../../src/OGL/VertexBuffer.cpp
    ../../src/OGL/VertexArray.cpp
    ../../src/OGL/IndexBuffer.cpp
    ../../src/Logger.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
