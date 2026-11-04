@echo off
REM ============================================================
REM  GPU Learning Project - build script
REM  Usage:  build.bat <subdir>          e.g. build.bat 00-hello-cpp
REM          build.bat <subdir> run      build then run
REM          build.bat <subdir> clean    remove build dir
REM  NOTE: keep this file ASCII-only. Chinese text breaks cmd.exe
REM        parsing under the GBK code page on zh-CN Windows.
REM
REM  Compiler location is auto-detected via vswhere, so the same
REM  script works on both machines:
REM    - office: VS 2026 Community    (CUDA 13.4, no GPU, compile-only)
REM    - home:   VS 2022 Build Tools  (CUDA 12.6, RTX 4070 Ti Super)
REM ============================================================
setlocal

REM --- locate Visual Studio / Build Tools ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDIR="
REM Home machine: VS 2022 Build Tools. nvcc 12.6 only supports host MSVC
REM up to VS 2022; the VS 2026 MSVC on this machine is rejected by nvcc.
if exist "E:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VSDIR=E:\BuildTools"
REM Fallback: any VS with MSVC found by vswhere (office machine: VS 2026).
if not defined VSDIR if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
)
if not defined VSDIR (
    echo [ERROR] MSVC not found. Install Visual Studio 2022 Build Tools:
    echo         winget install Microsoft.VisualStudio.2022.BuildTools
    exit /b 1
)
echo [INFO] Using compiler at: %VSDIR%

set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if "%~1"=="" (
    echo Usage: build.bat ^<subdir^> [run^|clean]
    echo Example: build.bat 00-hello-cpp run
    exit /b 1
)

set "SRCDIR=%~dp0code\%~1"
if not exist "%SRCDIR%\CMakeLists.txt" (
    echo [ERROR] Not found: %SRCDIR%\CMakeLists.txt
    exit /b 1
)

if /i "%~2"=="clean" (
    if exist "%SRCDIR%\build" rmdir /s /q "%SRCDIR%\build"
    echo [OK] Cleaned %SRCDIR%\build
    exit /b 0
)

REM --- load MSVC environment ---
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to load MSVC environment. Check vcvar64.bat under:
    echo         %VSDIR%\VC\Auxiliary\Build\
    exit /b 1
)

REM --- configure ---
"%CMAKE%" -S "%SRCDIR%" -B "%SRCDIR%\build" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

REM --- build ---
"%CMAKE%" --build "%SRCDIR%\build"
if errorlevel 1 exit /b 1

echo.
echo [OK] Build succeeded: %SRCDIR%\build\

REM --- optional run ---
if /i "%~2"=="run" (
    for %%F in ("%SRCDIR%\build\*.exe") do (
        echo.
        echo ---- Running %%F ----
        "%%F"
    )
)

endlocal
