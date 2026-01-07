@echo off
REM Compile Vulkan shaders to SPIR-V

echo Compiling Vulkan shaders...

set VULKAN_SDK=D:\dev\libs\vulkan
set GLSLC=%VULKAN_SDK%\Bin\glslangValidator.exe

echo.
echo [1/2] Compiling vertex shader...
"%GLSLC%" -V triangle.vert -o triangle.vert.spv
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile vertex shader
    pause
    exit /b 1
)

echo [2/2] Compiling fragment shader...
"%GLSLC%" -V triangle.frag -o triangle.frag.spv
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile fragment shader
    pause
    exit /b 1
)

echo.
echo All shaders compiled successfully!
echo.
dir *.spv
echo.
pause
