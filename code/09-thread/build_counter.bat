@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
cl /nologo -utf-8 /EHsc /W4 /O2 counter.cpp /Fe:counter.exe
if errorlevel 1 exit /b 1
"%~dp0counter.exe" %*
