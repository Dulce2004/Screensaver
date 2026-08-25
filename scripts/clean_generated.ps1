$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$build = Join-Path $root 'build'

if (Test-Path -LiteralPath $build) {
    $resolvedBuild = (Resolve-Path -LiteralPath $build).Path
    if (-not $resolvedBuild.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Ruta de build fuera del proyecto: $resolvedBuild"
    }
    Get-ChildItem -LiteralPath $resolvedBuild -Filter '*.exe' -File |
        ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
}

Get-ChildItem -LiteralPath $root -Filter '*.exe' -File |
    ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
Write-Host 'clean_generated=PASS'
