$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
Push-Location $root
try {
    $strict = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Iinclude', '-Isrc')
    & g++ @strict tests/core_behavior_test.cpp src/cli.cpp src/generation.cpp src/physics.cpp src/physics_openmp.cpp -o build/core_behavior_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'No compiló core_behavior_test.' }
    & .\build\core_behavior_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falló core_behavior_test.' }

    & g++ @strict '-DBUBBLES_ENABLE_OPENMP' '-fopenmp' tests/collision_test.cpp src/generation.cpp src/physics.cpp src/physics_openmp.cpp -o build/collision_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'No compiló collision_test.' }
    & .\build\collision_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falló collision_test.' }

    & g++ @strict '-DBUBBLES_ENABLE_OPENMP' '-fopenmp' tests/benchmark_fairness_test.cpp src/generation.cpp src/physics.cpp src/physics_openmp.cpp -o build/benchmark_fairness_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'No compiló benchmark_fairness_test.' }
    & .\build\benchmark_fairness_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Falló benchmark_fairness_test.' }

    & g++ '-std=c++17' '-Wall' '-Wextra' '-Wpedantic' '-Werror' '-Iinclude' '-fsyntax-only' tests/renderer_contract_test.cpp
    if ($LASTEXITCODE -ne 0) { throw 'Falló renderer_contract_test.' }

    & py -m unittest tests.analysis_test -v
    if ($LASTEXITCODE -ne 0) { throw 'Falló analysis_test.' }
    Write-Host 'tests=PASS'
}
finally {
    Pop-Location
}
