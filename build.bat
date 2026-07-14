@echo off
setlocal enabledelayedexpansion

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found. Please install Visual Studio 2022.
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo [ERROR] Visual Studio installation path not found.
    pause
    exit /b 1
)

set "MSBUILD=!VS_PATH!\MSBuild\Current\Bin\MSBuild.exe"
if not exist "!MSBUILD!" (
    echo [ERROR] MSBuild.exe not found at "!MSBUILD!".
    pause
    exit /b 1
)

echo [INFO] Building REvengeCS in Release x64...
"!MSBUILD!" REvengeCS.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Build succeeded! The executable is located at x64\Release\REvengeCS.exe
pause
