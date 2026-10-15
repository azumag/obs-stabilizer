param(
    [Parameter(Mandatory = $true)][string]$PluginPath,
    [Parameter(Mandatory = $true)][string]$ReportPath
)
$ErrorActionPreference = 'Stop'
# The caller supplies the exact target/package path. Never search by DLL name:
# dependency DLLs, stale builds, or another configuration must not satisfy this gate.
$plugin = (Resolve-Path -LiteralPath $PluginPath).Path
if ([IO.Path]::GetFileName($plugin) -cne 'obs-stabilizer-opencv.dll') {
    throw "Unexpected plugin filename: $plugin"
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'MSVC tools not found' }
$version = (Get-Content "$installation\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt").Trim()
$dumpbin = "$installation\VC\Tools\MSVC\$version\bin\Hostx64\x64\dumpbin.exe"
$exports = & $dumpbin /nologo /exports $plugin
if ($LASTEXITCODE -ne 0) { throw 'dumpbin /exports failed' }
$headers = & $dumpbin /nologo /headers $plugin
if ($LASTEXITCODE -ne 0) { throw 'dumpbin /headers failed' }
$imports = & $dumpbin /nologo /dependents $plugin
if ($LASTEXITCODE -ne 0) { throw 'dumpbin /dependents failed' }
$hash = (Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash
@("Plugin: $plugin", "SHA256: $hash", $exports, $headers, $imports) | Set-Content $ReportPath
if (-not ($headers -match '8664 machine \(x64\)')) { throw 'Expected an x64 plugin' }
foreach ($symbol in @('obs_module_load', 'obs_module_set_pointer', 'obs_module_ver',
                      'obs_module_name', 'obs_module_description', 'obs_module_unload')) {
    if (-not ($exports -cmatch "^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+$symbol\s*$")) {
        throw "Missing required OBS export: $symbol ($plugin)"
    }
}
if (-not ($imports -match '^\s+obs\.dll\s*$')) { throw 'Plugin must import the real obs.dll' }
Write-Host "Verified x64 OBS module: $plugin (SHA256 $hash)"
