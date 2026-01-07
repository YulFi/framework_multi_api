# Vulkan Shader Workflow

## Overview
The Vulkan renderer now uses **external SPIR-V shader files** instead of hardcoded bytecode.

## Shader Files Location
```
shaders/
├── triangle.vert          # GLSL vertex shader source
├── triangle.frag          # GLSL fragment shader source
├── triangle.vert.spv      # Compiled SPIR-V vertex shader (binary)
├── triangle.frag.spv      # Compiled SPIR-V fragment shader (binary)
├── compile_shaders.bat    # Helper script to compile shaders
└── README.md              # Detailed shader documentation
```

## Quick Start

### 1. Compile Shaders
Navigate to the `shaders` directory and run:
```cmd
compile_shaders.bat
```

Or use the Vulkan SDK tools directly:
```cmd
D:\sdk\vulkan1.4.304.0\Bin\glslangValidator.exe -V triangle.vert -o triangle.vert.spv
D:\sdk\vulkan1.4.304.0\Bin\glslangValidator.exe -V triangle.frag -o triangle.frag.spv
```

### 2. Build Project
The Visual Studio project is configured to automatically copy compiled shaders to the output directory.

Simply build the project in Visual Studio:
- **Build** → **Build Solution** (F7)

### 3. Run
```cmd
cd bin\Release
OpenGLProject.exe --plugin plugins\VKRenderer.dll
```

## Modifying Shaders

### Step-by-Step Process
1. **Edit** the GLSL source files (`triangle.vert`, `triangle.frag`)
2. **Compile** using `compile_shaders.bat` or the Vulkan SDK tools
3. **Rebuild** the Visual Studio project (shaders will be auto-copied)
4. **Run** the application

### Important Notes
- ✅ The Vulkan renderer loads `.spv` files, not `.vert`/`.frag` files
- ✅ Always recompile GLSL to SPIR-V after making changes
- ✅ The `.spv` files are binary - never edit them manually
- ✅ Post-build events auto-copy shaders to `bin\Release\shaders\` and `bin\Debug\shaders\`

## Shader Compilation Tools

### glslangValidator (Vulkan SDK)
```cmd
# Basic compilation
glslangValidator -V shader.vert -o shader.vert.spv

# With optimization
glslangValidator -V -Os shader.vert -o shader.vert.spv

# With debug info
glslangValidator -V -g shader.vert -o shader.vert.spv
```

### glslc (Alternative)
```cmd
# Basic compilation
glslc shader.vert -o shader.vert.spv

# With optimization
glslc -O shader.vert -o shader.vert.spv
```

## Troubleshooting

### "Failed to open shader file"
- Ensure `.spv` files exist in the `shaders` directory
- Check that the working directory is set correctly
- Verify shaders were copied to the executable's directory

### Shader Compilation Errors
- Check GLSL syntax (must be valid Vulkan GLSL)
- Ensure using `#version 450` or higher
- Verify input/output locations match between vertex and fragment shaders

### Runtime Crashes
- Validate shaders compiled successfully (check for `.spv` files)
- Ensure shader files are accessible from the executable
- Check that vertex input layout matches the shader expectations

## Current Shaders

### triangle.vert
Simple vertex shader that:
- Takes position (vec3) and color (vec3) as inputs
- Passes color to fragment shader
- Outputs position to clip space

### triangle.frag
Simple fragment shader that:
- Takes interpolated color from vertex shader
- Outputs final RGBA color

## Adding New Shaders

1. Create new GLSL files in `shaders/` directory
2. Compile to SPIR-V using the compilation tools
3. Update `Renderer.cpp` to load the new shaders:
   ```cpp
   auto vertCode = readShaderFile("shaders/myshader.vert.spv");
   auto fragCode = readShaderFile("shaders/myshader.frag.spv");
   ```
4. Update the post-build event in `OpenGLProject.vcxproj` to copy new shaders

## References
- [Vulkan GLSL Specification](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap45.html)
- [SPIR-V Tools](https://github.com/KhronosGroup/SPIRV-Tools)
- [Shader Compilation Guide](https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules)
