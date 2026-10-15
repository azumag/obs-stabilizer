function Get-WindowsPackageDllManifest {
    param(
        [Parameter(Mandatory = $true)][string]$ManifestPath
    )

    $names = @(
        Get-Content -LiteralPath $ManifestPath | ForEach-Object {
            $name = ([string]$_).Trim()
            if ($name -and -not $name.StartsWith('#')) {
                if ($name -notmatch '^[A-Za-z0-9_.-]+\.dll$') {
                    throw "Invalid Windows package DLL name in manifest: $name"
                }
                $name
            }
        }
    )
    if ($names.Count -eq 0) { throw "Windows package DLL manifest is empty: $ManifestPath" }

    $duplicates = @($names | Group-Object { $_.ToLowerInvariant() } | Where-Object Count -gt 1)
    if ($duplicates.Count -gt 0) {
        throw "Windows package DLL manifest has duplicate names: $($duplicates.Name -join ', ')"
    }
    return $names
}

function Assert-ExactWindowsPackageFileSet {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$ExpectedNames,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][string[]]$ActualNames,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $duplicates = @($ActualNames | Group-Object { $_.ToLowerInvariant() } | Where-Object Count -gt 1)
    if ($duplicates.Count -gt 0) {
        throw "$Context has duplicate names: $($duplicates.Name -join ', ')"
    }

    $missing = @($ExpectedNames | Where-Object { $_ -cnotin $ActualNames } | Sort-Object -CaseSensitive)
    $unexpected = @($ActualNames | Where-Object { $_ -cnotin $ExpectedNames } | Sort-Object -CaseSensitive)
    if ($missing.Count -gt 0 -or $unexpected.Count -gt 0) {
        throw "$Context DLL set differs from the allowlist. Missing: [$($missing -join ', ')]; unexpected: [$($unexpected -join ', ')]"
    }
}

Export-ModuleMember -Function Get-WindowsPackageDllManifest, Assert-ExactWindowsPackageFileSet
