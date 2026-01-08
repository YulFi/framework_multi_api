cmake_minimum_required(VERSION 3.15)
project(VKRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# Create shared library
add_library(${PROJECT_NAME} SHARED
    VKPlugin.cpp
    ../../src/VK/Renderer.cpp
    ../../src/VK/ShaderManager.cpp
    ../../src/VK/ShaderProgram.cpp
    ../../src/VK/VertexBuffer.cpp
    ../../src/VK/VertexArray.cpp
    ../../src/VK/IndexBuffer.cpp
    ../../src/VK/Texture.cpp
    ../../src/VK/ValidationLayers.cpp
    ../../src/TextureUtils.cpp
    ../../src/Logger.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glm::glm
    glfw
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
