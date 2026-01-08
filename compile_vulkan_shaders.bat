@echo off
echo Compiling Vulkan shaders...

REM Check if glslc is available
where glslc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using glslc compiler...
    glslc shaders/vulkan/basic.vert -o shaders/vulkan/basic.vert.spv
    glslc shaders/vulkan/basic.frag -o shaders/vulkan/basic.frag.spv
    echo Shaders compiled successfully!
) else (
    REM Try glslangValidator
    where glslangValidator >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo Using glslangValidator compiler...
        glslangValidator -V shaders/vulkan/basic.vert -o shaders/vulkan/basic.vert.spv
        glslangValidator -V shaders/vulkan/basic.frag -o shaders/vulkan/basic.frag.spv
        echo Shaders compiled successfully!
    ) else (
        echo ERROR: No Vulkan shader compiler found!
        echo Please install Vulkan SDK or add glslc/glslangValidator to PATH
        exit /b 1
    )
)

pause
