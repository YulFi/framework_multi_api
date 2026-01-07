# Vulkan Shaders

This directory contains GLSL shader source files and their compiled SPIR-V binaries for the Vulkan renderer.

## Shader Files

### Source Files (GLSL)
- `triangle.vert` - Vertex shader (simple pass-through with vertex colors)
- `triangle.frag` - Fragment shader (outputs interpolated vertex colors)

### Compiled Files (SPIR-V)
- `triangle.vert.spv` - Compiled vertex shader binary
- `triangle.frag.spv` - Compiled fragment shader binary

## Compiling Shaders

### Quick Method
Simply run the batch file:
```cmd
compile_shaders.bat
```

### Manual Compilation
Using `glslangValidator` (Vulkan SDK):
```cmd
D:\sdk\vulkan1.4.304.0\Bin\glslangValidator.exe -V triangle.vert -o triangle.vert.spv
D:\sdk\vulkan1.4.304.0\Bin\glslangValidator.exe -V triangle.frag -o triangle.frag.spv
```

Alternatively, using `glslc`:
```cmd
D:\sdk\vulkan1.4.304.0\Bin\glslc.exe triangle.vert -o triangle.vert.spv
D:\sdk\vulkan1.4.304.0\Bin\glslc.exe triangle.frag -o triangle.frag.spv
```

## Shader Details

### Vertex Shader (triangle.vert)
- **Inputs**:
  - `location 0`: vec3 position
  - `location 1`: vec3 color
- **Outputs**:
  - `location 0`: vec3 color (to fragment shader)
  - `gl_Position`: vec4 clip-space position

### Fragment Shader (triangle.frag)
- **Inputs**:
  - `location 0`: vec3 color (from vertex shader)
- **Outputs**:
  - `location 0`: vec4 final color (RGBA)

## Notes

- **IMPORTANT**: After modifying GLSL source files, you MUST recompile them to SPIR-V
- The Vulkan renderer loads `.spv` files at runtime, not `.vert`/`.frag` files
- SPIR-V files are binary and should not be edited manually
- Make sure to copy the compiled `.spv` files to your executable's directory or the shaders will fail to load
