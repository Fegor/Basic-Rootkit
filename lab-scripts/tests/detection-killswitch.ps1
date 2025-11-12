param(
    [Parameter(Mandatory)][string]$DllPath,
    [string]$TargetExecutable = 'notepad.exe',
    [string]$TargetArchitecture = 'x64',
    [string]$FilePrefix = '$pwn-detect',
    [int]$HarnessPort = 58888
)
$ErrorActionPreference = 'Stop'
if (-not (Get-Module -ListAvailable -Name Pester)) {
    throw 'Pester module is required. Install-Module Pester -Scope CurrentUser'
}
$modulePath = Join-Path $PSScriptRoot 'common.psm1'
Import-Module $modulePath -Force
$repoRoot = Get-LabRepoRoot

Describe 'Detection: kill switch restores visibility' {
    $procHandle = $null
    $testFile = $null
    $sessionId = $null

    BeforeAll {
        Assert-DllInBuildRegister -DllPath $DllPath -RepoRoot $repoRoot | Out-Null
        Invoke-HarnessApi -Method 'POST' -Path '/dll' -Port $HarnessPort -Body @{ path = $DllPath } | Out-Null
        $procHandle = Start-TestProcess -Executable $TargetExecutable -Prefix $FilePrefix
        $testFile = New-TestFile -Prefix $FilePrefix
        $body = @{ pid = $procHandle.Process.Id; architecture = $TargetArchitecture }
        $response = Invoke-HarnessApi -Method 'POST' -Path '/inject' -Port $HarnessPort -Body $body
        $sessionId = $response.sessionId
        Wait-ForProcessState -ImageName $procHandle.ImageName -ShouldBeVisible:$false | Out-Null
        Wait-ForFileState -Path $testFile -ShouldExist:$false | Out-Null
    }

    AfterAll {
        if ($procHandle) { Stop-TestProcess -Handle $procHandle }
        Remove-TestArtifacts | Out-Null
    }

    It 'restores visibility within 5 seconds' {
        $response = Invoke-HarnessApi -Method 'POST' -Path '/killswitch' -Port $HarnessPort -Body @{ sessionId = $sessionId }
        $response.restored | Should -BeTrue
        $response.elapsedMs | Should -BeLessThanOrEqual 5000
        (Wait-ForProcessState -ImageName $procHandle.ImageName -ShouldBeVisible:$true -TimeoutSec 5) | Should -BeTrue
        (Wait-ForFileState -Path $testFile -ShouldExist:$true -TimeoutSec 5) | Should -BeTrue
    }
}
