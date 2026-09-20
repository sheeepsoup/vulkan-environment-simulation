@echo off
setlocal
cd /d "%~dp0"

set "GLSLC=D:\Code\vulkan\SDK\Bin\glslc.exe"

if not exist "%GLSLC%" (
    echo [ERROR] glslc not found:
    echo %GLSLC%
    pause
    exit /b 1
)

echo Removing UTF-8 BOM...

for %%F in (
    "shader\simple_shader.vert"
    "shader\simple_shader.frag"
    "shader\compute.comp"
    "shader\slope.comp"
    "shader\shadow.vert"
) do (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
    "$path = [IO.Path]::GetFullPath('%%~F');" ^
    "$bytes = [IO.File]::ReadAllBytes($path);" ^
    "if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {" ^
    "  $result = New-Object byte[] ($bytes.Length - 3);" ^
    "  [Array]::Copy($bytes, 3, $result, 0, $result.Length);" ^
    "  [IO.File]::WriteAllBytes($path, $result);" ^
    "  Write-Host ('[BOM removed] ' + $path)" ^
    "} else {" ^
    "  Write-Host ('[No BOM]      ' + $path)" ^
    "}"
)

echo.
echo Compiling vertex shader...
"%GLSLC%" "shader\simple_shader.vert" -o "shader\simple_shader.vert.spv"
if errorlevel 1 goto compile_failed

echo Compiling fragment shader...
"%GLSLC%" "shader\simple_shader.frag" -o "shader\simple_shader.frag.spv"
if errorlevel 1 goto compile_failed

echo Compiling compute shader...
"%GLSLC%" "shader\compute.comp" -o "shader\compute.comp.spv"
if errorlevel 1 goto compile_failed

echo Compiling slope compute shader...
"%GLSLC%" "shader\slope.comp" -o "shader\slope.comp.spv"
if errorlevel 1 goto compile_failed

echo Compiling shadow vertex shader...
"%GLSLC%" "shader\shadow.vert" -o "shader\shadow.vert.spv"
if errorlevel 1 goto compile_failed

echo.
echo [SUCCESS] All shaders compiled successfully.
pause
exit /b 0

:compile_failed
echo.
echo [ERROR] Shader compilation failed.
pause
exit /b 1