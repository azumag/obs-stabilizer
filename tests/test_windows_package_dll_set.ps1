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

$expectedZipEntries = @(
    'obs-stabilizer/obs-plugins/64bit/obs-stabilizer-opencv.dll',
    'obs-stabilizer/obs-plugins/64bit/opencv_core4.dll'
)
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipEntries `
    -ActualNames $expectedZipEntries -Context 'forward-slash ZIP path fixture'

$backslashZipEntries = @($expectedZipEntries | ForEach-Object { $_.Replace('/', '\') })
$normalizedBackslashEntries = @($backslashZipEntries | ForEach-Object {
    ConvertTo-WindowsPackageZipPath -EntryPath $_
})
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipEntries `
    -ActualNames $normalizedBackslashEntries -Context 'backslash ZIP path fixture'

$mixedZipEntries = @($expectedZipEntries[0], $backslashZipEntries[1])
$normalizedMixedEntries = @($mixedZipEntries | ForEach-Object {
    ConvertTo-WindowsPackageZipPath -EntryPath $_
})
Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipEntries `
    -ActualNames $normalizedMixedEntries -Context 'mixed-separator ZIP path fixture'

$rejectedUnexpectedZipPath = $false
try {
    Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipEntries `
        -ActualNames (@($expectedZipEntries) + @('other/obs-stabilizer/obs-plugins/64bit/extra.dll')) `
        -Context 'unexpected ZIP path fixture'
} catch {
    if ($_.Exception.Message -notmatch 'unexpected: \[other/obs-stabilizer/obs-plugins/64bit/extra\.dll\]') {
        throw
    }
    $rejectedUnexpectedZipPath = $true
}
if (-not $rejectedUnexpectedZipPath) { throw 'The package gate accepted an unexpected ZIP path' }

$rejectedDuplicateZipPath = $false
$duplicateZipEntries = @($expectedZipEntries) + @($expectedZipEntries[0].Replace('/', '\'))
$normalizedDuplicateZipEntries = @($duplicateZipEntries | ForEach-Object {
    ConvertTo-WindowsPackageZipPath -EntryPath $_
})
try {
    Assert-ExactWindowsPackageFileSet -ExpectedNames $expectedZipEntries `
        -ActualNames $normalizedDuplicateZipEntries `
        -Context 'duplicate separator-normalized ZIP path fixture'
} catch {
    if ($_.Exception.Message -notmatch 'duplicate names') { throw }
    $rejectedDuplicateZipPath = $true
}
if (-not $rejectedDuplicateZipPath) { throw 'The package gate accepted duplicate separator-normalized ZIP paths' }

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
