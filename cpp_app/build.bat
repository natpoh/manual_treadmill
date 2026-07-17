@echo off
setlocal

cd %~dp0
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist build mkdir build
cd build

echo Configuring CMake...
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -G "Ninja" ..
if %errorlevel% neq 0 (
    echo [Fallback] Ninja failed, trying default MSBuild generator...
    "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ..
)

echo Building...
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config Release

echo Done. Executable is in build\Release\ (if MSBuild) or build\ (if Ninja).
