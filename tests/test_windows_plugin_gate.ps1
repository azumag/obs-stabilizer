$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Import-Module "$repo/scripts/ObsModuleExports.psm1" -Force
$normalExportRows = @('', 'ordinal hint RVA name', '') + @(
    '    1    0 00001000 obs_module_load',
    '    2    1 00001100 obs_module_set_pointer',
    '    3    2 00001200 obs_module_ver',
    '    4    3 00001300 obs_module_name',
    '    5    4 00001400 obs_module_description',
    '    6    5 00001500 obs_module_unload',
    ''
)
$iltExportRows = @('', 'ordinal hint RVA name', '') + @(
    '    1    0 00001000 obs_module_load = @ILT+1485(obs_module_load)',
    '    2    1 00001100 obs_module_set_pointer = @ILT+1720(obs_module_set_pointer)',
    '    3    2 00001200 obs_module_ver = @ILT+930(obs_module_ver)',
    '    4    3 00001300 obs_module_name = @ILT+985(obs_module_name)',
    '    5    4 00001400 obs_module_description = @ILT+1020(obs_module_description)',
    '    6    5 00001500 obs_module_unload = @ILT+1120(obs_module_unload)',
    ''
)
if (@(Get-MissingObsModuleExports -ExportTable $normalExportRows).Count -ne 0) {
    throw 'The export parser rejected ordinary dumpbin rows'
}
if (@(Get-MissingObsModuleExports -ExportTable $iltExportRows).Count -ne 0) {
    throw 'The export parser rejected dumpbin ILT annotations'
}
$similarNameRows = @('ordinal hint RVA name') + @(
    '    1    0 00001000 obs_module_load_helper = @ILT+1485(obs_module_load_helper)'
) + $normalExportRows[4..8]
$similarNameMissing = @(Get-MissingObsModuleExports -ExportTable $similarNameRows)
if ($similarNameMissing.Count -ne 1 -or $similarNameMissing[0] -cne 'obs_module_load') {
    throw 'A similar export name incorrectly satisfied obs_module_load'
}
$trulyMissingRows = @('ordinal hint RVA name') + @(
    '    1    0 00001000 obs_module_load',
    '    2    1 00001200 obs_module_ver',
    '    3    2 00001300 obs_module_name',
    '    4    3 00001400 obs_module_description',
    '    5    4 00001500 obs_module_unload'
)
$missingExports = @(Get-MissingObsModuleExports -ExportTable $trulyMissingRows)
if ($missingExports.Count -ne 1 -or $missingExports[0] -cne 'obs_module_set_pointer') {
    throw 'The export parser did not report a genuinely missing module export'
}
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
    $fixtureReport = "$fixture/fixture-report.txt"
    'Status: PASS (stale report fixture)' | Set-Content -LiteralPath $fixtureReport
    $rejected = $false
    try {
        & "$repo/scripts/verify-windows-plugin.ps1" `
            -PluginPath "$fixture/build/Release/obs-stabilizer-opencv.dll" `
            -ReportPath $fixtureReport
    } catch {
        if ($_.Exception.Message -notmatch 'Missing required OBS export: obs_module_set_pointer') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw 'Incomplete OBS DLL was accepted' }
    if (Test-Path -LiteralPath $fixtureReport) {
        throw 'Failed plugin verification left a stale PASS report'
    }
    Write-Host 'Windows gate regression passed: a load-only DLL is rejected at its exact path'
} finally {
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
