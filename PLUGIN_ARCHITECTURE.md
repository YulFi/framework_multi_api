# Plugin-Based Rendering Architecture

## Overview

The application now uses a **dynamic plugin system** for rendering backends. Each renderer (OpenGL, Vulkan, etc.) is compiled as a separate DLL that can be loaded at runtime.

## Benefits

1. **No Build Conflicts** - Each renderer is in its own project, eliminating file name collisions
2. **Hot-Swappable** - Switch renderers without recompiling the main application
3. **Smaller Executable** - Main app only loads what it needs
4. **Easy to Extend** - Add new renderers without modifying existing code
5. **Distribution Flexibility** - Ship only the renderers you support

## Architecture

```
Main Application
    ↓
PluginLoader → Loads DLL at runtime
    ↓
IRenderPlugin Interface
    ↓
Creates IRenderer + IShaderManager
    ↓
Application uses interfaces
```

## Directory Structure

```
E:\dev\test_claude2\
├── src/
│   ├── main.cpp                  (Entry point with plugin selection)
│   ├── Application.cpp            (Uses PluginLoader)
│   ├── MyApp.cpp                  (Your application logic)
│   ├── RenderAPI/
│   │   ├── IRenderer.h            (Renderer interface)
│   │   ├── IShaderManager.h       (Shader manager interface)
│   │   ├── IRenderPlugin.h        (Plugin interface)
│   │   ├── PluginLoader.h/cpp     (DLL loader)
│   ├── OGL/
│   │   ├── Renderer.h/cpp         (OpenGL implementation)
│   │   ├── ShaderManager.h/cpp
│   │   ├── VertexArray.h/cpp
│   │   └── VertexBuffer.h/cpp
│   └── VK/
│       ├── Renderer.h/cpp         (Vulkan stub)
│       └── ShaderManager.h/cpp
└── plugins/
    ├── OGLRenderer/
    │   ├── OGLPlugin.cpp          (Plugin entry point)
    │   └── CMakeLists.txt         (Builds OGLRenderer.dll)
    └── VKRenderer/
        ├── VKPlugin.cpp           (Plugin entry point)
        └── CMakeLists.txt         (Builds VKRenderer.dll)
```

## Building

### With CMake:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This builds:
- `OpenGLProject.exe` - Main executable
- `plugins/OGLRenderer.dll` - OpenGL renderer plugin
- `plugins/VKRenderer.dll` - Vulkan renderer plugin

## Running

### Default (OpenGL):
```bash
OpenGLProject.exe
```

### Specify Plugin:
```bash
# OpenGL
OpenGLProject.exe --plugin plugins/OGLRenderer.dll

# Vulkan
OpenGLProject.exe --plugin plugins/VKRenderer.dll
```

### Help:
```bash
OpenGLProject.exe --help
```

## Creating a New Renderer Plugin

### 1. Create Plugin Directory
```
plugins/YourRenderer/
├── YourPlugin.cpp
└── CMakeLists.txt
```

### 2. Implement Plugin Interface

**YourPlugin.cpp:**
```cpp
#define RENDER_PLUGIN_EXPORT
#include "../../src/RenderAPI/IRenderPlugin.h"
#include "YourRenderer.h"
#include "YourShaderManager.h"

class YourPlugin : public IRenderPlugin
{
public:
    const char* getName() const override
    {
        return "Your Renderer";
    }

    const char* getVersion() const override
    {
        return "1.0.0";
    }

    std::unique_ptr<IRenderer> createRenderer() override
    {
        return std::make_unique<YourRenderer>();
    }

    std::unique_ptr<IShaderManager> createShaderManager() override
    {
        return std::make_unique<YourShaderManager>();
    }
};

extern "C" {
    PLUGIN_API IRenderPlugin* CreatePlugin()
    {
        return new YourPlugin();
    }

    PLUGIN_API void DestroyPlugin(IRenderPlugin* plugin)
    {
        delete plugin;
    }
}
```

### 3. Implement IRenderer and IShaderManager

Your renderer must implement:
- `IRenderer` - All rendering operations
- `IShaderManager` - Shader loading and uniform setting

### 4. Add CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(YourRenderer)

add_library(${PROJECT_NAME} SHARED
    YourPlugin.cpp
    YourRenderer.cpp
    YourShaderManager.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
```

### 5. Register in Main CMakeLists.txt

Add to `CMakeLists.txt`:
```cmake
add_subdirectory(plugins/YourRenderer)
```

## Technical Details

### Plugin Loading (Windows)
- Uses `LoadLibraryA()` to load DLL
- Finds `CreatePlugin()` and `DestroyPlugin()` exports
- Calls `CreatePlugin()` to get IRenderPlugin instance

### Plugin Loading (Linux/Mac)
- Uses `dlopen()` to load .so/.dylib
- Finds exported functions with `dlsym()`
- Same interface as Windows

### Memory Management
- Plugins create objects with `new`
- Main app manages with `std::unique_ptr`
- Plugin's `DestroyPlugin()` ensures proper cleanup

## Current Plugins

### OGLRenderer.dll
- **Full OpenGL 3.3 implementation**
- Vertex arrays, buffers, shaders
- Production-ready

### VKRenderer.dll
- **Vulkan stub for demonstration**
- Logs operations to console
- Framework ready for full implementation

## Troubleshooting

### "Failed to load plugin"
- Check DLL is in `plugins/` directory
- Verify all dependencies are available
- Check Windows Event Viewer for load errors

### "Plugin loaded but could not get instance"
- Ensure `CreatePlugin()` is exported
- Verify plugin implements IRenderPlugin

### Build Warnings
- Ignore MSB8027 warnings - they're resolved by plugin architecture
