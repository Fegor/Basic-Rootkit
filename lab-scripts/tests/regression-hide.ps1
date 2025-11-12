param(
    [Parameter(Mandatory)][string]$DllPath,
    [string]$TargetExecutable = 'notepad.exe',
    [string]$TargetArchitecture = 'x64',
    [string]$FilePrefix = '$pwn-regression',
    [int]$HarnessPort = 58888
)
$ErrorActionPreference = 'Stop'
if (-not (Get-Module -ListAvailable -Name Pester)) {
    throw 'Pester module is required. Install-Module Pester -Scope CurrentUser'
}
$modulePath = Join-Path $PSScriptRoot 'common.psm1'
Import-Module $modulePath -Force
$repoRoot = Get-LabRepoRoot

Describe 'Regression: hiding processes and files' {
    $procHandle = $null
    $testFile = $null
    $sessionId = $null

    BeforeAll {
        Assert-DllInBuildRegister -DllPath $DllPath -RepoRoot $repoRoot | Out-Null
        Invoke-HarnessApi -Method 'POST' -Path '/dll' -Port $HarnessPort -Body @{ path = $DllPath } | Out-Null
        $procHandle = Start-TestProcess -Executable $TargetExecutable -Prefix $FilePrefix
        $testFile = New-TestFile -Prefix $FilePrefix
    }

    AfterAll {
        if ($sessionId) {
            try { Invoke-HarnessApi -Method 'POST' -Path '/killswitch' -Port $HarnessPort -Body @{ sessionId = $sessionId } | Out-Null } catch {}
        }
        if ($procHandle) { Stop-TestProcess -Handle $procHandle }
        Remove-TestArtifacts | Out-Null
    }

    It 'injects DLL and returns HiddenVerified' {
        $body = @{ pid = $procHandle.Process.Id; architecture = $TargetArchitecture }
        $response = Invoke-HarnessApi -Method 'POST' -Path '/inject' -Port $HarnessPort -Body $body
        $sessionId = $response.sessionId
        $response.status | Should -Be 'HiddenVerified'
    }

    It 'hides the prefixed process' {
        (Wait-ForProcessState -ImageName $procHandle.ImageName -ShouldBeVisible:$false -TimeoutSec 5) | Should -BeTrue
    }

    It 'hides the prefixed file entry' {
        (Wait-ForFileState -Path $testFile -ShouldExist:$false -TimeoutSec 5) | Should -BeTrue
    }
}
