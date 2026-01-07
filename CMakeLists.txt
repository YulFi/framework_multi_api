cmake_minimum_required(VERSION 3.15)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages installed via vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

# Main executable (no renderer code, just plugin loader)
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MyApp.cpp
    src/Application.cpp
    src/WindowManager.cpp
    src/Camera.cpp
    src/Logger.cpp
    src/RenderAPI/PluginLoader.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
    Vulkan::Vulkan
)

# Add plugin subdirectories
add_subdirectory(plugins/OGLRenderer)
add_subdirectory(plugins/VKRenderer)

# Set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
