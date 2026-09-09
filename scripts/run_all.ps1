param(
    [switch]$Overwrite,
    [switch]$SkipFps
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    & .\scripts\build.ps1
    & .\scripts\run_tests.ps1
    $overwriteFlag = if ($Overwrite) { @('--overwrite') } else { @() }
    & py .\scripts\run_benchmark_campaign.py @overwriteFlag
    if ($LASTEXITCODE -ne 0) { throw 'Falló la campaña del benchmark.' }
    & py .\scripts\analyze_benchmarks.py
    if ($LASTEXITCODE -ne 0) { throw 'Falló el análisis del benchmark.' }
    if (-not $SkipFps) {
        & py .\scripts\run_fps_campaign.py @overwriteFlag
        if ($LASTEXITCODE -ne 0) { throw 'Falló la campaña FPS.' }
        & py .\scripts\analyze_fps.py
        if ($LASTEXITCODE -ne 0) { throw 'Falló el análisis FPS.' }
    }
    & py .\scripts\capture_measurement.py
    if ($LASTEXITCODE -ne 0) { throw 'Falló la captura de medición.' }
    & py .\scripts\build_report.py
    if ($LASTEXITCODE -ne 0) { throw 'Falló la construcción del informe.' }
}
finally {
    Pop-Location
}
