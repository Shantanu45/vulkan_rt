# vulkan_rt

`vulkan_rt` is a modern C++ CMake project template. It is intentionally small at
the application layer and more complete at the build-system layer, so it can be
used as a starting point for a real executable project.

## What is included

- CMake presets for MSVC and clang-cl on Windows, and GCC/Clang on Unix-like systems.
- Conan-based third-party dependency setup.
- Interface targets for project warnings and project options.
- Optional hardening, sanitizers, IPO/LTO, unity builds, precompiled headers, ccache,
  clang-tidy, cppcheck, coverage, and fuzz testing.
- A small CLI executable using CLI11, fmt, and spdlog.
- Catch2 unit tests and CTest smoke tests for `--help` and `--version`.
- Optional WebAssembly support for Emscripten builds.

## Requirements

- CMake 3.29 or newer.
- Conan 2.x.
- Ninja, or another CMake generator if you customize the presets.
- A C++ compiler with C++23 support.

## Configure and build

On Windows, the simplest path is the wrapper script:

```bat
build.bat
```

That defaults to `windows-msvc-debug` and runs Conan install, CMake configure,
build, and tests. You can also choose a preset and action:

```bat
build.bat windows-msvc-release build
build.bat windows-clang-debug test
```

To generate a Visual Studio solution instead of a Ninja build tree:

```bat
GenerateVisualStudioProject.bat
```

Then open:

```text
out/build/vs-Debug/vulkan_rt.sln
```

List the available presets:

```sh
cmake --list-presets
```

Configure and build on Windows with MSVC:

```sh
conan profile detect --force
conan install . --output-folder=out/build/windows-msvc-debug --build=missing -s build_type=Debug -s compiler.cppstd=23
out\build\windows-msvc-debug\conanbuild.bat
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

Configure and build on Windows with clang-cl:

```sh
conan profile detect --force
conan install . --output-folder=out/build/windows-clang-debug --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset windows-clang-debug
cmake --build --preset windows-clang-debug
```

On Linux or macOS, use one of the Unix-like presets:

```sh
conan profile detect --force
conan install . --output-folder=out/build/unixlike-clang-debug --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset unixlike-clang-debug
cmake --build --preset unixlike-clang-debug
```

On Windows with MSVC, run from a Visual Studio Developer Prompt or call the
generated `conanbuild.bat` before configuring/building so `cl.exe`, `link.exe`,
`rc.exe`, and `mt.exe` are all on `PATH`.

## Test

```sh
ctest --preset test-windows-msvc-debug
```

or run CTest directly from a build directory:

```sh
ctest --test-dir out/build/windows-msvc-debug --output-on-failure
```

## Project layout

- `src/` contains the executable target.
- `include/` contains public example headers.
- `test/` contains Catch2 tests.
- `fuzz_test/` contains a libFuzzer target.
- `configured_files/` contains generated build metadata headers.
- `cmake/` contains reusable CMake helper modules.

## Renaming the template

For a new project, run:

```powershell
.\RenameProject.ps1 MyNewProject
```

The script normalizes the name to a CMake/C++ friendly identifier. For example,
`My New Project` becomes `my_new_project`. To choose a different C++ namespace
and include folder:

```powershell
.\RenameProject.ps1 MyNewProject myapp
```

Preview the changes without writing files:

```powershell
.\RenameProject.ps1 MyNewProject -DryRun
```
