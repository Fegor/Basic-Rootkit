param(
    [Parameter(Mandatory)][string]$DllPath,
    [int]$HarnessPort = 58888
)
$ErrorActionPreference = 'Stop'
if (-not (Get-Module -ListAvailable -Name Pester)) {
    throw 'Pester module is required. Install-Module Pester -Scope CurrentUser'
}
$modulePath = Join-Path $PSScriptRoot 'common.psm1'
Import-Module $modulePath -Force
$repoRoot = Get-LabRepoRoot

Describe 'DLL contract surface' {
    It 'tracks the DLL in build-register.json' {
        $entry = Assert-DllInBuildRegister -DllPath $DllPath -RepoRoot $repoRoot
        $entry.sha256 | Should -Not -BeNullOrEmpty
    }

    It 'exposes /processes endpoint' {
        { Invoke-HarnessApi -Method 'GET' -Path '/processes' -Port $HarnessPort } | Should -Not -Throw
    }

    It 'rejects injection for non-existent process' {
        { Invoke-HarnessApi -Method 'POST' -Path '/inject' -Port $HarnessPort -Body @{ pid = 999999; architecture = 'x64' } } | Should -Throw
    }
}
