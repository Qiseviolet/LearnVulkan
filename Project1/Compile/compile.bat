@echo off
setlocal ENABLEDELAYEDEXPANSION

echo =============================
echo   Vulkan Shader Compiler
echo =============================
echo.

REM 1. 获取当前 bat 文件所在目录
set BASE_DIR=%~dp0
echo [INFO] BAT File Directory:
echo        %BASE_DIR%
echo.

REM 2. 设置 glslc 路径（相对于 BASE_DIR）
set GLSLC=%BASE_DIR%..\Libraries\Vulkan\Bin\glslc.exe
echo [INFO] GLSLC Full Path:
echo        %GLSLC%
echo.

REM 3. Shader 源文件路径
set SHADER_DIR=%BASE_DIR%..\Shader\
echo [INFO] Shader Directory:
echo        %SHADER_DIR%
echo.

echo -----------------------------
echo Compiling Vertex Shader...
echo Command: "%GLSLC%" "%SHADER_DIR%shader.vert" -o "%SHADER_DIR%vert.spv"
echo -----------------------------
"%GLSLC%" "%SHADER_DIR%shader.vert" -o "%SHADER_DIR%vert.spv"

echo.
echo -----------------------------
echo Compiling Fragment Shader...
echo Command: "%GLSLC%" "%SHADER_DIR%shader.frag" -o "%SHADER_DIR%frag.spv"
echo -----------------------------
"%GLSLC%" "%SHADER_DIR%shader.frag" -o "%SHADER_DIR%frag.spv"

echo.
echo =============================
echo Compilation Complete.
echo =============================
pause