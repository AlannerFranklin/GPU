@echo off
REM Build + run MyVector exam test (MSVC, direct cl)
setlocal
call "E:\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 ( echo [ERROR] vcvars64 failed & exit /b 1 )
cl /nologo /EHsc /utf-8 /std:c++17 /W4 myvector_exam_test.cpp /Fe:myvector_exam_test.exe
if errorlevel 1 ( echo [ERROR] compile failed & exit /b 1 )
echo.
.\myvector_exam_test.exe
endlocal
