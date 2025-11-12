Set-StrictMode -Version Latest

function Get-LabRepoRoot {
    param([string]$StartDir = (Get-Location).Path)
    $current = Resolve-Path $StartDir
    while ($true) {
        if (Test-Path (Join-Path $current '.specify')) { return $current }
        $parent = Split-Path $current -Parent
        if ($parent -eq $current) { throw "Cannot locate repo root from $StartDir" }
        $current = $parent
    }
}

function Get-BuildRegister {
    param([string]$RepoRoot = $(Get-LabRepoRoot))
    $path = Join-Path $RepoRoot 'lab-output/build-register.json'
    if (-not (Test-Path $path)) { throw "Missing build register at $path" }
    return Get-Content -Raw -Path $path | ConvertFrom-Json
}

function Assert-DllInBuildRegister {
    param([Parameter(Mandatory)][string]$DllPath,[string]$RepoRoot = $(Get-LabRepoRoot))
    $register = Get-BuildRegister -RepoRoot $RepoRoot
    $full = [System.IO.Path]::GetFullPath($DllPath)
    if (-not (Test-Path $full)) { throw "DLL path '$DllPath' does not exist" }
    $relative = $full.Substring($RepoRoot.Length).TrimStart('\\','/')
    $entry = $register | Where-Object { $_.artifact -eq $relative }
    if (-not $entry) { throw "DLL '$relative' missing from build-register.json" }
    return $entry
}

function Invoke-HarnessApi {
    param(
        [ValidateSet('GET','POST','DELETE')] [string]$Method = 'GET',
        [string]$Path = '/',
        [hashtable]$Body,
        [int]$Port = 58888,
        [int]$TimeoutSec = 5
    )
    $uri = "http://localhost:$Port$Path"
    $params = @{ Method = $Method; Uri = $uri; TimeoutSec = $TimeoutSec }
    if ($Body) {
        $params.ContentType = 'application/json'
        $params.Body = ($Body | ConvertTo-Json -Depth 6)
    }
    return Invoke-RestMethod @params
}

function Wait-ForProcessState {
    param([Parameter(Mandatory)][string]$ImageName,[bool]$ShouldBeVisible,[int]$TimeoutSec = 5)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $exists = Get-Process -Name $ImageName -ErrorAction SilentlyContinue | ForEach-Object { $true } | Select-Object -First 1
        if (($exists -eq $true) -eq $ShouldBeVisible) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Wait-ForFileState {
    param(
        [Parameter(Mandatory)][string]$Path,
        [bool]$ShouldExist,
        [int]$TimeoutSec = 5
    )
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    $directory = Split-Path -Path $Path -Parent
    $name = Split-Path -Path $Path -Leaf
    while ((Get-Date) -lt $deadline) {
        $exists = (Get-ChildItem -LiteralPath $directory -ErrorAction SilentlyContinue | Where-Object { Set-StrictMode -Version Latest

function Get-LabRepoRoot {
    param([string]$StartDir = (Get-Location).Path)
    $current = Resolve-Path $StartDir
    while ($true) {
        if (Test-Path (Join-Path $current '.specify')) { return $current }
        $parent = Split-Path $current -Parent
        if ($parent -eq $current) { throw "Cannot locate repo root from $StartDir" }
        $current = $parent
    }
}

function Get-BuildRegister {
    param([string]$RepoRoot = $(Get-LabRepoRoot))
    $path = Join-Path $RepoRoot 'lab-output/build-register.json'
    if (-not (Test-Path $path)) { throw "Missing build register at $path" }
    return Get-Content -Raw -Path $path | ConvertFrom-Json
}

function Assert-DllInBuildRegister {
    param([Parameter(Mandatory)][string]$DllPath,[string]$RepoRoot = $(Get-LabRepoRoot))
    $register = Get-BuildRegister -RepoRoot $RepoRoot
    $full = [System.IO.Path]::GetFullPath($DllPath)
    if (-not (Test-Path $full)) { throw "DLL path '$DllPath' does not exist" }
    $relative = $full.Substring($RepoRoot.Length).TrimStart('\\','/')
    $entry = $register | Where-Object { $_.artifact -eq $relative }
    if (-not $entry) { throw "DLL '$relative' missing from build-register.json" }
    return $entry
}

function Invoke-HarnessApi {
    param(
        [ValidateSet('GET','POST','DELETE')] [string]$Method = 'GET',
        [string]$Path = '/',
        [hashtable]$Body,
        [int]$Port = 58888,
        [int]$TimeoutSec = 5
    )
    $uri = "http://localhost:$Port$Path"
    $params = @{ Method = $Method; Uri = $uri; TimeoutSec = $TimeoutSec }
    if ($Body) {
        $params.ContentType = 'application/json'
        $params.Body = ($Body | ConvertTo-Json -Depth 6)
    }
    return Invoke-RestMethod @params
}

function Wait-ForProcessState {
    param([Parameter(Mandatory)][string]$ImageName,[bool]$ShouldBeVisible,[int]$TimeoutSec = 5)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $exists = Get-Process -Name $ImageName -ErrorAction SilentlyContinue | ForEach-Object { $true } | Select-Object -First 1
        if (($exists -eq $true) -eq $ShouldBeVisible) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Wait-ForFileState {
    param([Parameter(Mandatory)][string]$Path,[bool]$ShouldExist,[int]$TimeoutSec = 5)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $exists = Test-Path $Path
        if ($exists -eq $ShouldExist) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Start-TestProcess {
    param(
        [string]$Executable = 'notepad.exe',
        [string]$Prefix = '$pwn'
    )
    $source = (Get-Command $Executable).Source
    $tempDir = Join-Path $env:TEMP 'pwn-procs'
    if (-not (Test-Path $tempDir)) { New-Item -ItemType Directory -Path $tempDir | Out-Null }
    $random = [System.Guid]::NewGuid().ToString('N')
    $fileName = "$Prefix$random-$([System.IO.Path]::GetFileName($source))"
    $renamed = Join-Path $tempDir $fileName
    Copy-Item -Path $source -Destination $renamed -Force
    $proc = Start-Process $renamed -PassThru
    Start-Sleep -Milliseconds 500
    return [pscustomobject]@{ Process = $proc; ImageName = [System.IO.Path]::GetFileNameWithoutExtension($renamed) }
}

function Stop-TestProcess {
    param([Parameter(Mandatory)][pscustomobject]$Handle)
    if ($Handle.Process -and -not $Handle.Process.HasExited) {
        $Handle.Process.Kill()
        $Handle.Process.WaitForExit()
    }
}

function New-TestFile {
    param([string]$Directory = (Join-Path $env:TEMP 'pwn-files'),[string]$Prefix = '$pwn')
    if (-not (Test-Path $Directory)) { New-Item -ItemType Directory -Path $Directory | Out-Null }
    $path = Join-Path $Directory "$Prefix$(Get-Random)-test.txt"
    'test' | Set-Content -Path $path -Encoding UTF8
    return $path
}

function Remove-TestArtifacts {
    param([string]$FileDirectory = (Join-Path $env:TEMP 'pwn-files'),[string]$ProcessDir = (Join-Path $env:TEMP 'pwn-procs'))
    if (Test-Path $FileDirectory) { Remove-Item -Recurse -Force -Path $FileDirectory }
    if (Test-Path $ProcessDir) { Remove-Item -Recurse -Force -Path $ProcessDir }
}

Export-ModuleMember -Function *
.Name -eq $name }) -ne $null
        if ($exists -eq $ShouldExist) { return $true }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Start-TestProcess {
    param(
        [string]$Executable = 'notepad.exe',
        [string]$Prefix = '$pwn'
    )
    $source = (Get-Command $Executable).Source
    $tempDir = Join-Path $env:TEMP 'pwn-procs'
    if (-not (Test-Path $tempDir)) { New-Item -ItemType Directory -Path $tempDir | Out-Null }
    $random = [System.Guid]::NewGuid().ToString('N')
    $fileName = "$Prefix$random-$([System.IO.Path]::GetFileName($source))"
    $renamed = Join-Path $tempDir $fileName
    Copy-Item -Path $source -Destination $renamed -Force
    $proc = Start-Process $renamed -PassThru
    Start-Sleep -Milliseconds 500
    return [pscustomobject]@{ Process = $proc; ImageName = [System.IO.Path]::GetFileNameWithoutExtension($renamed) }
}

function Stop-TestProcess {
    param([Parameter(Mandatory)][pscustomobject]$Handle)
    if ($Handle.Process -and -not $Handle.Process.HasExited) {
        $Handle.Process.Kill()
        $Handle.Process.WaitForExit()
    }
}

function New-TestFile {
    param([string]$Directory = (Join-Path $env:TEMP 'pwn-files'),[string]$Prefix = '$pwn')
    if (-not (Test-Path $Directory)) { New-Item -ItemType Directory -Path $Directory | Out-Null }
    $path = Join-Path $Directory "$Prefix$(Get-Random)-test.txt"
    'test' | Set-Content -Path $path -Encoding UTF8
    return $path
}

function Remove-TestArtifacts {
    param([string]$FileDirectory = (Join-Path $env:TEMP 'pwn-files'),[string]$ProcessDir = (Join-Path $env:TEMP 'pwn-procs'))
    if (Test-Path $FileDirectory) { Remove-Item -Recurse -Force -Path $FileDirectory }
    if (Test-Path $ProcessDir) { Remove-Item -Recurse -Force -Path $ProcessDir }
}

Export-ModuleMember -Function *

