@echo off
setlocal

set CONFIG=Release
if "%~1"=="debug" (
    set CONFIG=Debug
)
if "%~1"=="Debug" (
    set CONFIG=Debug
)

cd %~dp0
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist build mkdir build
cd build

echo Configuring CMake for %CONFIG%...
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -G "Ninja" -DCMAKE_BUILD_TYPE=%CONFIG% ..
if %errorlevel% neq 0 (
    echo [Fallback] Ninja failed, trying default MSBuild generator...
    "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ..
)

echo Building %CONFIG%...
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build . --config %CONFIG%

echo Done. Executable is in build\Release\ or build\Debug\ (if MSBuild) or build\ (if Ninja).
