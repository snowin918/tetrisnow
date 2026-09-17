@echo off
setlocal enabledelayedexpansion

REM Builds Tetrisnow with CMake+Ninja+MSVC. Usage:
REM   build.bat            (builds both Debug and Release)
REM   build.bat debug      (Debug only  -> build\Tetrisnow.exe)
REM   build.bat release    (Release only -> build-release\Tetrisnow.exe)
REM
REM Locates Visual Studio (for cl.exe/link.exe) and its bundled CMake/Ninja
REM via vswhere, so this doesn't depend on any of those being on PATH.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Could not find vswhere.exe -- is Visual Studio installed?
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo Could not find a Visual Studio install with the C++ toolset ^(Microsoft.VisualStudio.Component.VC.Tools.x86.x64^).
    exit /b 1
)

set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo vcvars64.bat not found under %VSINSTALL%
    exit /b 1
)
call "%VCVARS%" >nul

where cmake >nul 2>nul
if errorlevel 1 (
    set "PATH=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
)

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=both"

if /I "%TARGET%"=="debug" (
    call :build Debug build
    exit /b !errorlevel!
) else if /I "%TARGET%"=="release" (
    call :build Release build-release
    exit /b !errorlevel!
) else if /I "%TARGET%"=="both" (
    call :build Debug build
    if !errorlevel! neq 0 exit /b 1
    call :build Release build-release
    exit /b !errorlevel!
) else (
    echo Usage: %~nx0 [debug^|release^|both]
    exit /b 1
)

:build
set "CFG=%~1"
set "DIR=%~2"
echo.
echo === %CFG% (%DIR%) ===
cmake -S . -B "%DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%CFG%
if !errorlevel! neq 0 exit /b 1
cmake --build "%DIR%"
exit /b !errorlevel!
