$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null

$sources = @(
    'src/main.cpp', 'src/cli.cpp', 'src/csv.cpp', 'src/generation.cpp',
    'src/physics.cpp', 'src/physics_openmp.cpp', 'src/renderer.cpp',
    'src/benchmark.cpp', 'src/visual.cpp'
)
$common = @(
    '-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
    '-I', (Join-Path $root 'include'), '-isystem', (Join-Path $root 'third_party')
)
$libraries = @('-lglfw3', '-lglew32', '-lopengl32', '-lgdi32')

& g++ @common @sources '-o' (Join-Path $build 'BubbleScreensaver.exe') @libraries
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación secuencial.' }
& g++ @common '-DBUBBLES_ENABLE_OPENMP' '-fopenmp' @sources `
    '-o' (Join-Path $build 'BubbleScreensaverOpenMP.exe') @libraries
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación OpenMP.' }
Write-Host 'build=PASS (sequential, OpenMP)'
