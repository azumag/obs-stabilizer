$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path $repo 'tmp/windows-plugin-gate'
New-Item -ItemType Directory -Force "$fixture/source" | Out-Null
@'
cmake_minimum_required(VERSION 3.16)
project(incomplete_plugin C)
add_library(obs-stabilizer-opencv MODULE incomplete.c incomplete.def)
'@ | Set-Content "$fixture/source/CMakeLists.txt"
# A DLL exporting only load must fail even if a complete plugin is present in
# build/RelWithDebInfo. The .def alias also matches dumpbin's `= @ILT` suffix.
# The verifier must inspect only the supplied fixture.
'int obs_module_load_impl(void) { return 1; }' |
    Set-Content "$fixture/source/incomplete.c"
@'
EXPORTS
    obs_module_load=obs_module_load_impl
'@ | Set-Content "$fixture/source/incomplete.def"
try {
    cmake -S "$fixture/source" -B "$fixture/build" -A x64
    if ($LASTEXITCODE -ne 0) { throw 'Fixture configure failed' }
    cmake --build "$fixture/build" --config Release
    if ($LASTEXITCODE -ne 0) { throw 'Fixture build failed' }
    $rejected = $false
    try {
        & "$repo/scripts/verify-windows-plugin.ps1" `
            -PluginPath "$fixture/build/Release/obs-stabilizer-opencv.dll" `
            -ReportPath "$fixture/fixture-report.txt"
    } catch {
        if ($_.Exception.Message -notmatch 'Missing required OBS export: obs_module_set_pointer') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw 'Incomplete OBS DLL was accepted' }
    Write-Host 'Windows gate regression passed: a load-only DLL is rejected at its exact path'
} finally {
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
