$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Import-Module "$repo/scripts/WindowsPackageFiles.psm1" -Force

$manifest = Get-WindowsPackageDllManifest -ManifestPath "$repo/scripts/windows-package-dlls.txt"
if ($manifest.Count -ne 22 -or $manifest -cnotcontains 'obs-stabilizer-opencv.dll') {
    throw 'The Windows package manifest does not describe the verified 22-DLL payload'
}
Assert-ExactWindowsPackageFileSet -ExpectedNames $manifest -ActualNames $manifest `
    -Context 'matching package DLL fixture'

$rejectedMissing = $false
try {
    Assert-ExactWindowsPackageFileSet -ExpectedNames $manifest `
        -ActualNames @($manifest | Where-Object { $_ -cne 'libprotobuf.dll' }) `
        -Context 'missing DLL fixture'
} catch {
    if ($_.Exception.Message -notmatch 'Missing: \[libprotobuf\.dll\]') { throw }
    $rejectedMissing = $true
}
if (-not $rejectedMissing) { throw 'The package allowlist accepted a missing runtime DLL' }

$rejectedUnexpected = $false
try {
    Assert-ExactWindowsPackageFileSet -ExpectedNames $manifest `
        -ActualNames (@($manifest) + @('unapproved-runtime.dll')) `
        -Context 'unexpected DLL fixture'
} catch {
    if ($_.Exception.Message -notmatch 'unexpected: \[unapproved-runtime\.dll\]') { throw }
    $rejectedUnexpected = $true
}
if (-not $rejectedUnexpected) { throw 'The package allowlist accepted an unexpected DLL' }

$rejectedCaseChange = $false
try {
    Assert-ExactWindowsPackageFileSet -ExpectedNames @('obs-stabilizer-opencv.dll') `
        -ActualNames @('OBS-STABILIZER-OPENCV.dll') -Context 'case-sensitive filename fixture'
} catch {
    $rejectedCaseChange = $true
}
if (-not $rejectedCaseChange) { throw 'The package allowlist accepted a changed DLL filename' }

$temporaryManifest = Join-Path ([System.IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString() + '.txt')
try {
    @('example.dll', 'EXAMPLE.dll') | Set-Content -LiteralPath $temporaryManifest
    $rejectedDuplicate = $false
    try {
        Get-WindowsPackageDllManifest -ManifestPath $temporaryManifest | Out-Null
    } catch {
        if ($_.Exception.Message -notmatch 'duplicate names') { throw }
        $rejectedDuplicate = $true
    }
    if (-not $rejectedDuplicate) { throw 'The package manifest accepted case-insensitive duplicates' }
} finally {
    Remove-Item -LiteralPath $temporaryManifest -Force -ErrorAction SilentlyContinue
}

Write-Host 'Windows package DLL allowlist regression passed'
