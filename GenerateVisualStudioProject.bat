@echo off
setlocal

set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Debug"

set "VS_GENERATOR=%~2"
if "%VS_GENERATOR%"=="" set "VS_GENERATOR=Visual Studio 17 2022"

set "ARCH=%~3"
if "%ARCH%"=="" set "ARCH=x64"

if /I "%BUILD_TYPE%"=="help" goto :help
if /I "%BUILD_TYPE%"=="--help" goto :help
if /I "%BUILD_TYPE%"=="/?" goto :help

set "BUILD_DIR=out\build\vs-%BUILD_TYPE%"
set "CONAN_COMPILER_VERSION=194"
if not "%VS_GENERATOR:2019=%"=="%VS_GENERATOR%" set "CONAN_COMPILER_VERSION=193"
if not "%VS_GENERATOR:2017=%"=="%VS_GENERATOR%" set "CONAN_COMPILER_VERSION=191"

if exist "%BUILD_DIR%" cmake -E rm -rf "%BUILD_DIR%" || exit /b 1

conan profile detect --force || exit /b 1
conan install . --output-folder="%BUILD_DIR%" --build=missing -s build_type=%BUILD_TYPE% -s compiler=msvc -s compiler.version=%CONAN_COMPILER_VERSION% -s compiler.cppstd=23 -c tools.cmake.cmaketoolchain:generator="%VS_GENERATOR%" || exit /b 1
if exist CMakeUserPresets.json cmake -E rm -f CMakeUserPresets.json

call "%BUILD_DIR%\conanbuild.bat" || exit /b 1

cmake -S . -B "%BUILD_DIR%" -G "%VS_GENERATOR%" -A "%ARCH%" -DCMAKE_TOOLCHAIN_FILE="%BUILD_DIR%\conan_toolchain.cmake" || exit /b 1

set "SLN_PATH="
for %%F in ("%BUILD_DIR%\*.sln") do set "SLN_PATH=%%~fF"

echo.
echo Visual Studio solution generated:
if defined SLN_PATH (
  echo   %SLN_PATH%
) else (
  echo   %BUILD_DIR%\*.sln
)
exit /b 0

:help
echo Usage:
echo   GenerateVisualStudioProject.bat [build-type] [generator] [arch]
echo.
echo Defaults:
echo   build-type: Debug
echo   generator:  Visual Studio 17 2022
echo   arch:       x64
echo.
echo Examples:
echo   GenerateVisualStudioProject.bat
echo   GenerateVisualStudioProject.bat Release
echo   GenerateVisualStudioProject.bat Debug "Visual Studio 17 2022" x64
exit /b 0
