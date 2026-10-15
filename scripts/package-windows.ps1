param([string]$BuildDir = 'build', [string]$Configuration = 'RelWithDebInfo')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Import-Module "$PSScriptRoot/WindowsPackageFiles.psm1" -Force
$build = (Resolve-Path -LiteralPath $BuildDir).Path
$packageReport = Join-Path $build 'windows-package-dlls-report.txt'
foreach ($report in @($packageReport, (Join-Path $build 'windows-plugin-package.txt'))) {
    if (Test-Path -LiteralPath $report -PathType Leaf) {
        Remove-Item -LiteralPath $report -Force
    }
}
$expectedDllNames = @(Get-WindowsPackageDllManifest -ManifestPath "$PSScriptRoot/windows-package-dlls.txt")
$pluginName = 'obs-stabilizer-opencv.dll'
if ($expectedDllNames -cnotcontains $pluginName) { throw "The DLL allowlist omits the OBS plugin: $pluginName" }
$plugin = Join-Path $build "$Configuration/obs-stabilizer-opencv.dll"
& "$PSScriptRoot/verify-windows-plugin.ps1" -PluginPath $plugin -ReportPath "$build/windows-plugin-build.txt"
$stage = Join-Path $build 'windows-package'
$check = Join-Path $build 'windows-package-check'
foreach ($directory in @($stage, $check)) {
    if (Test-Path $directory) { Remove-Item -Recurse -Force $directory }
}
$destination = Join-Path $stage 'obs-stabilizer/obs-plugins/64bit'
New-Item -ItemType Directory -Force $destination | Out-Null
$sourceDllDirectory = Join-Path $build $Configuration
$buildDlls = @(Get-ChildItem -LiteralPath $sourceDllDirectory -Filter '*.dll' -File | Sort-Object Name)
# OBS supplies these host libraries, while GTest DLLs only serve the test target.
# Exact exclusions ensure that a new build output cannot disappear behind a broad pattern.
$nonPackageBuildDllNames = @('gtest.dll', 'gtest_main.dll', 'obs.dll', 'w32-pthreads.dll')
$payloadBuildDlls = @($buildDlls | Where-Object { $_.Name -cnotin $nonPackageBuildDllNames })
$excludedBuildDlls = @($buildDlls | Where-Object { $_.Name -cin $nonPackageBuildDllNames } | ForEach-Object Name)
Write-Host "Windows build DLL outputs: $($buildDlls.Name -join ', ')"
Write-Host "Windows build DLL exclusions: $($excludedBuildDlls -join ', ')"
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedDllNames `
    -ActualNames @($payloadBuildDlls | ForEach-Object Name) `
    -Context 'Windows build output'
$payloadBuildDlls | Copy-Item -Destination $destination
$stagedDllNames = @(Get-ChildItem -LiteralPath $destination -Filter '*.dll' -File | ForEach-Object Name)
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedDllNames -ActualNames $stagedDllNames `
    -Context 'staged Windows package'
Copy-Item -LiteralPath "$repo/README.md", "$repo/LICENSE" -Destination "$stage/obs-stabilizer"
$archive = Join-Path $build 'obs-stabilizer-windows.zip'
Compress-Archive -Path "$stage/obs-stabilizer" -DestinationPath $archive -Force

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipFile = [System.IO.Compression.ZipFile]::OpenRead($archive)
try {
    $expectedZipDllPaths = @($expectedDllNames | ForEach-Object { "obs-stabilizer/obs-plugins/64bit/$_" })
    $actualZipDllPaths = @($zipFile.Entries | Where-Object { $_.FullName -match '\.dll$' } | ForEach-Object FullName)
    Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipDllPaths -ActualNames $actualZipDllPaths `
        -Context 'Windows ZIP'
} finally {
    $zipFile.Dispose()
}

Expand-Archive -LiteralPath $archive -DestinationPath $check
$shippedDllDirectory = Join-Path $check 'obs-stabilizer/obs-plugins/64bit'
$shippedDlls = @(Get-ChildItem -LiteralPath $shippedDllDirectory -Filter '*.dll' -File | Sort-Object Name)
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedDllNames `
    -ActualNames @($shippedDlls | ForEach-Object Name) -Context 'extracted Windows ZIP'
$shipped = Join-Path $shippedDllDirectory $pluginName
$pluginCopies = @(Get-ChildItem $check -Recurse -Filter 'obs-stabilizer*.dll')
if ($pluginCopies.Count -ne 1) { throw 'Expected exactly one stabilizer plugin DLL in ZIP' }
& "$PSScriptRoot/verify-windows-plugin.ps1" -PluginPath $shipped -ReportPath "$build/windows-plugin-package.txt"

$reportLines = @(
    'Status: PASS',
    "ZIP: $archive",
    'DLL payload (filename and SHA256):'
)
foreach ($name in $expectedDllNames) {
    $sourcePath = Join-Path $sourceDllDirectory $name
    $shippedPath = Join-Path $shippedDllDirectory $name
    $sourceHash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash
    $shippedHash = (Get-FileHash -LiteralPath $shippedPath -Algorithm SHA256).Hash
    if ($sourceHash -cne $shippedHash) {
        throw "Packaged DLL differs from its exact build target: $name"
    }
    $reportLines += "$name SHA256 $shippedHash"
}
$reportLines | Set-Content -LiteralPath $packageReport
Get-FileHash -Algorithm SHA256 $archive | Format-List
