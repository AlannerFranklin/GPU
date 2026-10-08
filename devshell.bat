@echo off
REM ============================================================
REM  Open a cmd shell with the MSVC environment loaded.
REM  Use this when you want to type cl / link / lib / dumpbin
REM  by hand (which is what Week 1 is all about).
REM
REM  Usage:  devshell.bat            (starts in the GPU dir)
REM          devshell.bat code\02-my-first-build
REM
REM  NOTE: ASCII only. Chinese breaks cmd.exe parsing under GBK.
REM ============================================================
setlocal

set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community"

call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to load MSVC environment.
    exit /b 1
)

echo.
echo ============================================================
echo   MSVC developer shell ready.
echo ============================================================
where cl.exe
echo.
echo   Try:  cl /?     link /?     dumpbin /?     lib /?
echo.

if "%~1"=="" (
    cd /d "%~dp0"
) else (
    if exist "%~dp0%~1" (
        cd /d "%~dp0%~1"
    ) else (
        echo [WARN] Directory not found: %~dp0%~1
        echo        Staying in %~dp0
        cd /d "%~dp0"
    )
)

echo   Current directory: %CD%
echo.

REM Keep the shell open and interactive
cmd /k
