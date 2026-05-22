@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=windows-msvc-debug"

set "ACTION=%~2"
if "%ACTION%"=="" set "ACTION=test"

set "BUILD_TYPE=Debug"
if /I "%PRESET%"=="windows-msvc-release" set "BUILD_TYPE=RelWithDebInfo"
if /I "%PRESET%"=="windows-clang-release" set "BUILD_TYPE=RelWithDebInfo"

set "BUILD_DIR=out\build\%PRESET%"
set "TEST_PRESET=test-%PRESET%"

if /I "%PRESET%"=="help" goto :help
if /I "%PRESET%"=="--help" goto :help
if /I "%PRESET%"=="/?" goto :help

if /I "%ACTION%"=="configure" goto :configure
if /I "%ACTION%"=="build" goto :build
if /I "%ACTION%"=="test" goto :test
if /I "%ACTION%"=="clean" goto :clean

echo Unknown action: %ACTION%
echo.
goto :help

:configure
call :conan_install || exit /b 1
call "%BUILD_DIR%\conanbuild.bat" || exit /b 1
cmake --preset "%PRESET%" || exit /b 1
exit /b 0

:build
call :configure || exit /b 1
cmake --build --preset "%PRESET%" || exit /b 1
exit /b 0

:test
call :build || exit /b 1
ctest --preset "%TEST_PRESET%" || exit /b 1
exit /b 0

:clean
if exist "%BUILD_DIR%" (
  cmake -E rm -rf "%BUILD_DIR%" || exit /b 1
)
exit /b 0

:conan_install
conan profile detect --force || exit /b 1
conan install . --output-folder="%BUILD_DIR%" --build=missing -s build_type=%BUILD_TYPE% -s compiler.cppstd=23 -c tools.cmake.cmaketoolchain:generator=Ninja || exit /b 1
if exist CMakeUserPresets.json cmake -E rm -f CMakeUserPresets.json
exit /b 0

:help
echo Usage:
echo   build.bat [preset] [action]
echo.
echo Presets:
echo   windows-msvc-debug      default
echo   windows-msvc-release
echo   windows-clang-debug
echo   windows-clang-release
echo.
echo Actions:
echo   configure               run Conan install and CMake configure
echo   build                   configure and build
echo   test                    configure, build, and test ^(default^)
echo   clean                   remove the preset build directory
echo.
echo Examples:
echo   build.bat
echo   build.bat windows-msvc-release build
echo   build.bat windows-clang-debug test
exit /b 0
