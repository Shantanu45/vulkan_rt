param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Name,

    [Parameter(Position = 1)]
    [string]$Namespace,

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Convert-ToSafeIdentifier([string]$Value) {
    $safe = $Value.ToLowerInvariant() -replace '[^a-z0-9_]+', '_'
    $safe = $safe.Trim('_')
    if ($safe -match '^[0-9]') {
        $safe = "project_$safe"
    }
    return $safe
}

function Set-TextFileContent([string]$Path, [string]$Content) {
    if ($DryRun) {
        Write-Host "Would update $Path"
        return
    }

    Set-Content -LiteralPath $Path -Value $Content -NoNewline
}

$ProjectName = Convert-ToSafeIdentifier $Name
if ([string]::IsNullOrWhiteSpace($ProjectName)) {
    throw "Project name '$Name' does not contain any usable identifier characters."
}

if ([string]::IsNullOrWhiteSpace($Namespace)) {
    $Namespace = $ProjectName
}

$SafeNamespace = Convert-ToSafeIdentifier $Namespace
if ([string]::IsNullOrWhiteSpace($SafeNamespace)) {
    throw "Namespace '$Namespace' does not contain any usable identifier characters."
}

$Root = Split-Path -Parent $PSCommandPath
$OldProjectName = "cmake_template_project"
$OldNamespace = "myproject"
$OldHeaderGuardPrefix = "myproject"
$NewHeaderGuardPrefix = $SafeNamespace.ToUpperInvariant()

$textFiles = @(
    ".github/constants.env",
    ".github/workflows/ci.yml",
    ".github/workflows/codeql-analysis.yml",
    ".github/workflows/template-janitor.yml",
    ".gitlab-ci.yml",
    "CMakeLists.txt",
    "CMakePresets.json",
    "Dependencies.cmake",
    "ProjectOptions.cmake",
    "README.md",
    "README_building.md",
    "README_dependencies.md",
    "README_docker.md",
    "build.bat",
    "GenerateVisualStudioProject.bat",
    "configured_files/config.hpp.in",
    "docs/Doxyfile",
    "include/$OldNamespace/sample_library.hpp",
    "src/CMakeLists.txt",
    "src/main.cpp",
    "test/CMakeLists.txt",
    "test/test_main.cpp",
    "web/shell_template.html.in",
    "cmake/Cache.cmake",
    "cmake/CompilerWarnings.cmake",
    "cmake/Cuda.cmake",
    "cmake/Doxygen.cmake",
    "cmake/Emscripten.cmake",
    "cmake/Hardening.cmake",
    "cmake/InterproceduralOptimization.cmake",
    "cmake/LibFuzzer.cmake",
    "cmake/Linker.cmake",
    "cmake/PackageProject.cmake",
    "cmake/PreventInSourceBuilds.cmake",
    "cmake/Sanitizers.cmake",
    "cmake/StandardProjectSettings.cmake",
    "cmake/StaticAnalyzers.cmake",
    "cmake/Tests.cmake",
    "cmake/VCEnvironment.cmake"
)

foreach ($relative in $textFiles) {
    $path = Join-Path $Root $relative
    if (-not (Test-Path -LiteralPath $path)) {
        continue
    }

    $content = Get-Content -LiteralPath $path -Raw
    $content = $content.Replace($OldProjectName, $ProjectName)
    $content = $content.Replace($OldNamespace, $SafeNamespace)
    $content = $content.Replace($OldHeaderGuardPrefix.ToUpperInvariant(), $NewHeaderGuardPrefix)
    Set-TextFileContent $path $content
}

$oldIncludeDir = Join-Path $Root "include\$OldNamespace"
$newIncludeDir = Join-Path $Root "include\$SafeNamespace"
if ((Test-Path -LiteralPath $oldIncludeDir) -and ($oldIncludeDir -ne $newIncludeDir)) {
    if ($DryRun) {
        Write-Host "Would move $oldIncludeDir to $newIncludeDir"
    } else {
        if (Test-Path -LiteralPath $newIncludeDir) {
            throw "Cannot move include directory: '$newIncludeDir' already exists."
        }
        Move-Item -LiteralPath $oldIncludeDir -Destination $newIncludeDir
    }
}

Write-Host "Project name: $ProjectName"
Write-Host "C++ namespace / include folder: $SafeNamespace"

if ($DryRun) {
    Write-Host "Dry run complete; no files were changed."
} else {
    Write-Host "Rename complete."
    Write-Host "Next: run 'build.bat windows-msvc-debug test' to verify."
}
