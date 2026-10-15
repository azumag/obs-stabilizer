$script:RequiredObsModuleExports = @(
    'obs_module_load',
    'obs_module_set_pointer',
    'obs_module_ver',
    'obs_module_name',
    'obs_module_description',
    'obs_module_unload'
)

function Get-ObsModuleExportNames {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string[]]$ExportTable)

    foreach ($line in $ExportTable) {
        # Parse the name column, not a substring of the full line. Dumpbin may
        # append an ILT/alias annotation after `=`, which is not part of the
        # exported name.
        if ($line -cmatch '^\s+\d+\s+\d+\s+[0-9a-fA-F]+\s+(?<name>\S+)(?:\s+=\s+.*)?\s*$') {
            $Matches['name']
        }
    }
}

function Get-MissingObsModuleExports {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string[]]$ExportTable)

    $names = @(Get-ObsModuleExportNames -ExportTable $ExportTable)
    foreach ($required in $script:RequiredObsModuleExports) {
        if ($names -cnotcontains $required) { $required }
    }
}

Export-ModuleMember -Function Get-ObsModuleExportNames, Get-MissingObsModuleExports
