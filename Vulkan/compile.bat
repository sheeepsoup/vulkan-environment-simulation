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

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$files = @('shader\simple_shader.vert','shader\simple_shader.frag','shader\compute.comp','shader\slope.comp','shader\shadow.vert','shader\spectrum.comp'); foreach ($relativePath in $files) { $path = Join-Path (Get-Location) $relativePath; if (-not (Test-Path -LiteralPath $path)) { Write-Host ('[Missing]     ' + $relativePath); continue }; $bytes = [System.IO.File]::ReadAllBytes($path); if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) { $result = New-Object byte[] ($bytes.Length - 3); [Array]::Copy($bytes, 3, $result, 0, $result.Length); [System.IO.File]::WriteAllBytes($path, $result); Write-Host ('[BOM removed] ' + $relativePath) } else { Write-Host ('[No BOM]      ' + $relativePath) } }"

echo.
echo Compiling vertex shader...
"%GLSLC%" "shader\simple_shader.vert" -o "shader\simple_shader.vert.spv"
if errorlevel 1 goto compile_failed

echo Compiling fragment shader...
"%GLSLC%" "shader\simple_shader.frag" -o "shader\simple_shader.frag.spv"
if errorlevel 1 goto compile_failed

echo Compiling erosion compute shader...
"%GLSLC%" "shader\compute.comp" -o "shader\compute.comp.spv"
if errorlevel 1 goto compile_failed

echo Compiling slope compute shader...
"%GLSLC%" "shader\slope.comp" -o "shader\slope.comp.spv"
if errorlevel 1 goto compile_failed

echo Compiling shadow vertex shader...
"%GLSLC%" "shader\shadow.vert" -o "shader\shadow.vert.spv"
if errorlevel 1 goto compile_failed

echo Compiling spectrum compute shader...
"%GLSLC%" "shader\spectrum.comp" -o "shader\spectrum.comp.spv"
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