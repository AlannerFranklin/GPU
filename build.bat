@echo off
REM ============================================================
REM  GPU Learning Project - build script
REM  Usage:  build.bat <subdir>          e.g. build.bat 00-hello-cpp
REM          build.bat <subdir> run      build then run
REM          build.bat <subdir> clean    remove build dir
REM  NOTE: keep this file ASCII-only. Chinese text breaks cmd.exe
REM        parsing under the GBK code page on zh-CN Windows.
REM ============================================================
setlocal

set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
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
    echo [ERROR] Failed to load MSVC environment. Check the VS install path.
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
