@echo off
setlocal

set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Debug"

set "BUILD_DIR=out\build\vs-%BUILD_TYPE%"

:: Fallback default if VS_GENERATOR is completely empty
if "%VS_GENERATOR%"=="" set "VS_GENERATOR=Visual Studio 17 2022"

:: Default Conan compiler version for VS 2022
set "CONAN_COMPILER_VERSION=194"

:: Safely check for VS 2019 or 2017 using quotes to prevent syntax crashes
if not "%VS_GENERATOR:2019=%"=="%VS_GENERATOR%" set "CONAN_COMPILER_VERSION=193"
if not "%VS_GENERATOR:2017=%"=="%VS_GENERATOR%" set "CONAN_COMPILER_VERSION=191"

conan profile detect --force || exit /b 1
conan install . --output-folder="%BUILD_DIR%" --build=missing -s build_type=%BUILD_TYPE% -s compiler=msvc -s compiler.version=%CONAN_COMPILER_VERSION% -s compiler.cppstd=23 -c tools.cmake.cmaketoolchain:generator="%VS_GENERATOR%" || exit /b 1

pause
