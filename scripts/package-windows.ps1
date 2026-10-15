param([string]$BuildDir = 'build', [string]$Configuration = 'RelWithDebInfo')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = (Resolve-Path -LiteralPath $BuildDir).Path
$plugin = Join-Path $build "$Configuration/obs-stabilizer-opencv.dll"
& "$PSScriptRoot/verify-windows-plugin.ps1" -PluginPath $plugin -ReportPath "$build/windows-plugin-build.txt"
$stage = Join-Path $build 'windows-package'
$check = Join-Path $build 'windows-package-check'
foreach ($directory in @($stage, $check)) {
    if (Test-Path $directory) { Remove-Item -Recurse -Force $directory }
}
$destination = Join-Path $stage 'obs-stabilizer/obs-plugins/64bit'
New-Item -ItemType Directory -Force $destination | Out-Null
# Keep the existing vcpkg app-local runtime DLLs, excluding the unit test and
# host OBS libraries. OBS supplies its own obs.dll; never replace that runtime.
Get-ChildItem -LiteralPath "$build/$Configuration" -Filter '*.dll' |
    Where-Object { $_.Name -notmatch '^(gtest.*|obs|w32-pthreads)\.dll$' } |
    Copy-Item -Destination $destination
Copy-Item -LiteralPath "$repo/README.md", "$repo/LICENSE" -Destination "$stage/obs-stabilizer"
$archive = Join-Path $build 'obs-stabilizer-windows.zip'
Compress-Archive -Path "$stage/obs-stabilizer" -DestinationPath $archive -Force
Expand-Archive -LiteralPath $archive -DestinationPath $check
$shipped = Join-Path $check 'obs-stabilizer/obs-plugins/64bit/obs-stabilizer-opencv.dll'
if ((Get-FileHash $plugin).Hash -ne (Get-FileHash $shipped).Hash) {
    throw 'Packaged DLL differs from the verified build target'
}
$pluginCopies = @(Get-ChildItem $check -Recurse -Filter 'obs-stabilizer*.dll')
if ($pluginCopies.Count -ne 1) { throw 'Expected exactly one stabilizer plugin DLL in ZIP' }
& "$PSScriptRoot/verify-windows-plugin.ps1" -PluginPath $shipped -ReportPath "$build/windows-plugin-package.txt"
Get-FileHash -Algorithm SHA256 $archive | Format-List
