@echo off
setlocal ENABLEDELAYEDEXPANSION

echo =============================
echo   Vulkan Shader Compiler
echo =============================
echo.

REM Get current bat file directory
set BASE_DIR=%~dp0
echo [INFO] BAT File Directory:
echo        %BASE_DIR%
echo.

REM Set glslc path relative to BASE_DIR
set GLSLC=%BASE_DIR%..\Libraries\Vulkan\Bin\glslc.exe
echo [INFO] GLSLC Full Path:
echo        %GLSLC%
echo.

REM Shader source file directory
set SHADER_DIR=%BASE_DIR%..\Shader\
echo [INFO] Shader Directory:
echo        %SHADER_DIR%
echo.

echo -----------------------------
echo Compiling Vertex Shader...
echo Command: "%GLSLC%" -fshader-stage=vertex "%SHADER_DIR%shader.vert" -o "%SHADER_DIR%vert.spv"
echo -----------------------------
"%GLSLC%" -fshader-stage=vertex "%SHADER_DIR%shader.vert" -o "%SHADER_DIR%vert.spv"
if errorlevel 1 (
    echo ERROR: Failed to compile vertex shader!
    pause
    exit /b 1
)

echo.
echo -----------------------------
echo Compiling Fragment Shader...
echo Command: "%GLSLC%" -fshader-stage=fragment "%SHADER_DIR%shader.frag" -o "%SHADER_DIR%frag.spv"
echo -----------------------------
"%GLSLC%" -fshader-stage=fragment "%SHADER_DIR%shader.frag" -o "%SHADER_DIR%frag.spv"
if errorlevel 1 (
    echo ERROR: Failed to compile fragment shader!
    pause
    exit /b 1
)

echo.
echo -----------------------------
echo Compiling Shadow Vertex Shader...
echo Command: "%GLSLC%" -fshader-stage=vertex "%SHADER_DIR%shadow.vert" -o "%SHADER_DIR%shadow_vert.spv"
echo -----------------------------
"%GLSLC%" -fshader-stage=vertex "%SHADER_DIR%shadow.vert" -o "%SHADER_DIR%shadow_vert.spv"
if errorlevel 1 (
    echo ERROR: Failed to compile shadow vertex shader!
    pause
    exit /b 1
)

echo.
echo -----------------------------
echo Compiling Shadow Fragment Shader...
echo Command: "%GLSLC%" -fshader-stage=fragment "%SHADER_DIR%shadow.frag" -o "%SHADER_DIR%shadow_frag.spv"
echo -----------------------------
"%GLSLC%" -fshader-stage=fragment "%SHADER_DIR%shadow.frag" -o "%SHADER_DIR%shadow_frag.spv"
if errorlevel 1 (
    echo ERROR: Failed to compile shadow fragment shader!
    pause
    exit /b 1
)

echo.
echo =============================
echo Compilation Complete.
echo =============================
pause
