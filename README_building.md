## Build Instructions

A full build has different steps:
1) Specifying the compiler using environment variables
2) Installing dependencies with Conan
3) Configuring the project
4) Building the project

For the subsequent builds, in case you change the source code, you only need to repeat the last step.

### (1) Specify the compiler using environment variables

By default (if you don't set environment variables `CC` and `CXX`), the system default compiler will be used.

CMake uses the environment variables CC and CXX to decide which compiler to use. So to avoid the conflict issues only specify the compilers using these variables.


<details>
<summary>Commands for setting the compilers </summary>

- Debian/Ubuntu/MacOS:

	Set your desired compiler (`clang`, `gcc`, etc):

	- Temporarily (only for the current shell)

		Run one of the followings in the terminal:

		- clang

				CC=clang CXX=clang++

		- gcc

				CC=gcc CXX=g++

	- Permanent:

		Open `~/.bashrc` using your text editor:

			gedit ~/.bashrc

		Add `CC` and `CXX` to point to the compilers:

			export CC=clang
			export CXX=clang++

		Save and close the file.

- Windows:

	- Permanent:

		Run one of the followings in PowerShell:

		- Visual Studio generator and compiler (cl)

				[Environment]::SetEnvironmentVariable("CC", "cl.exe", "User")
				[Environment]::SetEnvironmentVariable("CXX", "cl.exe", "User")
				refreshenv

		  Set the architecture using [vcvarsall](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019#vcvarsall-syntax):

				vcvarsall.bat x64

		- clang

				[Environment]::SetEnvironmentVariable("CC", "clang.exe", "User")
				[Environment]::SetEnvironmentVariable("CXX", "clang++.exe", "User")
				refreshenv

		- gcc

				[Environment]::SetEnvironmentVariable("CC", "gcc.exe", "User")
				[Environment]::SetEnvironmentVariable("CXX", "g++.exe", "User")
				refreshenv


  - Temporarily (only for the current shell):

			$Env:CC="clang.exe"
			$Env:CXX="clang++.exe"

</details>

### (2) Install dependencies with Conan

The project uses Conan 2 with `CMakeDeps` and `CMakeToolchain`.

On Windows, `build.bat` wraps the normal Conan and CMake steps:

```shell
build.bat windows-msvc-debug test
```

To generate a Visual Studio solution:

```shell
GenerateVisualStudioProject.bat Debug
```

The solution is written to `out/build/vs-Debug/vulkan_rt.sln`.

### Using this repository as a template

Rename the template before starting feature work:

```shell
powershell -ExecutionPolicy Bypass -File RenameProject.ps1 MyNewProject
```

Then verify:

```shell
build.bat windows-msvc-debug test
```

Create or refresh your default Conan profile:

```shell
conan profile detect --force
```

Install dependencies into the same directory CMake will use for the build:

```shell
conan install . --output-folder=out/build/windows-msvc-debug --build=missing -s build_type=Debug -s compiler.cppstd=23
```

Use the output folder that matches the CMake preset you plan to configure.
On Windows with MSVC, run from a Visual Studio Developer Prompt or call the
generated `conanbuild.bat` before configuring/building.

### (3) Configure your build

To configure the project, you could use `cmake`, or `ccmake` or `cmake-gui`. Each of them are explained in the following:

#### (3.a) Configuring via cmake:
With Cmake directly:

    conan install . --output-folder=./build --build=missing -s build_type=Debug -s compiler.cppstd=23
    cmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=./build/conan_toolchain.cmake

Cmake will automatically create the `./build` folder if it does not exist, and it wil configure the project.

Instead, if you have CMake version 3.21+, you can use one of the configuration presets that are listed in the CmakePresets.json file.

    cmake . --preset <configure-preset>
    cmake --build

#### (3.b) Configuring via ccmake:

With the Cmake Curses Dialog Command Line tool:

    ccmake -S . -B ./build -DCMAKE_TOOLCHAIN_FILE=./build/conan_toolchain.cmake

Once `ccmake` has finished setting up, press 'c' to configure the project,
press 'g' to generate, and 'q' to quit.

#### (3.c) Configuring via cmake-gui:

To use the GUI of the cmake:

2.c.1) Open cmake-gui from the project directory:
```
cmake-gui .
```
2.c.2) Set the build directory:

![build_dir](https://user-images.githubusercontent.com/16418197/82524586-fa48e380-9af4-11ea-8514-4e18a063d8eb.jpg)

2.c.3) Configure the generator:

In cmake-gui, from the upper menu select `Tools/Configure`.

**Warning**: if you have set `CC` and `CXX` always choose the `use default native compilers` option. This picks `CC` and `CXX`. Don't change the compiler at this stage!

<details>
<summary>Windows - MinGW Makefiles</summary>

Choose MinGW Makefiles as the generator:

<img src="https://user-images.githubusercontent.com/16418197/82769479-616ade80-9dfa-11ea-899e-3a8c31d43032.png" alt="mingw">

</details>

<details>
<summary>Windows - Visual Studio generator and compiler</summary>

You should have already set `C` and `CXX` to `cl.exe`.

Choose "Visual Studio 16 2019" as the generator:

<img src="https://user-images.githubusercontent.com/16418197/82524696-32502680-9af5-11ea-9697-a42000e900a6.jpg" alt="default_vs">

</details>

<details>

<summary>Windows - Visual Studio generator and Clang Compiler</summary>

You should have already set `C` and `CXX` to `clang.exe` and `clang++.exe`.

Choose "Visual Studio 16 2019" as the generator. To tell Visual studio to use `clang-cl.exe`:
- If you use the LLVM that is shipped with Visual Studio: write `ClangCl` under "optional toolset to use".

<img src="https://user-images.githubusercontent.com/16418197/82781142-ae60ac00-9e1e-11ea-8c77-222b005a8f7e.png" alt="visual_studio">

- If you use an external LLVM: write [`LLVM_v142`](https://github.com/zufuliu/llvm-utils#llvm-for-visual-studio-2017-and-2019)
 under "optional toolset to use".

<img src="https://user-images.githubusercontent.com/16418197/82769558-b3136900-9dfa-11ea-9f73-02ab8f9b0ca4.png" alt="visual_studio">

</details>
<br/>

2.c.4) Choose the Cmake options and then generate:

![generate](https://user-images.githubusercontent.com/16418197/82781591-c97feb80-9e1f-11ea-86c8-f2748b96f516.png)

### (4) Build the project
Once you have selected all the options you would like to use, you can build the
project (all targets):

    cmake --build ./build

For Visual Studio, give the build configuration (Release, RelWithDeb, Debug, etc) like the following:

    cmake --build ./build -- /p:configuration=Release


### Running the tests

You can use the `ctest` command run the tests.

```shell
cd ./build
ctest -C Debug
cd ../
```
