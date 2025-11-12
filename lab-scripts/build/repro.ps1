param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x86', 'x64')]
    [string]$Platform = 'x86',
    [string]$SemVer = '0.1.0-dev'
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent # .../lab-scripts
$repoRoot = Split-Path -Parent $repoRoot
Set-Location $repoRoot

function Write-Info($msg) { Write-Host "[repro] $msg" }

$solutions = @("Source/Basic Rootkit.sln")
if (Test-Path "Source/MfcHarness/MfcHarness.sln") {
    $solutions += "Source/MfcHarness/MfcHarness.sln"
}

$gitCommit = 'unknown'
try { $gitCommit = (git rev-parse HEAD).Trim() } catch {}
$gitStatus = 'dirty'
try { if (-not (git status --short)) { $gitStatus = 'clean' } } catch {}

$msbuild = "msbuild"
$buildStart = Get-Date
foreach ($sln in $solutions) {
    if (-not (Test-Path $sln)) {
        Write-Info "Skip missing solution $sln"
        continue
    }
    Write-Info "Building $sln [$Configuration|$Platform]"
    & $msbuild $sln /m /p:Configuration=$Configuration /p:Platform=$Platform /p:Deterministic=true /p:EmbedUntrackedSources=true /p:UseSharedCompilation=false /nologo | Write-Host
}

$artifactRoot = Join-Path $repoRoot 'lab-output'
if (-not (Test-Path $artifactRoot)) { New-Item -ItemType Directory -Path $artifactRoot | Out-Null }

$artifacts = Get-ChildItem -Path $repoRoot -Include *.dll,*.exe -Recurse |
    Where-Object { $_.LastWriteTime -ge $buildStart }

$register = @()
foreach ($file in $artifacts) {
    $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
    $relative = $file.FullName.Substring($repoRoot.Length).TrimStart('\','/')
    $register += [pscustomobject]@{
        artifact = $relative
        configuration = $Configuration
        platform = $Platform
        semver = $SemVer
        gitCommit = $gitCommit
        gitStatus = $gitStatus
        sha256 = $hash
        buildTimeUtc = (Get-Date).ToUniversalTime().ToString('o')
    }
}

$registerPath = Join-Path $artifactRoot 'build-register.json'
$register | ConvertTo-Json -Depth 4 | Set-Content -Path $registerPath -Encoding UTF8
Write-Info "Wrote $($register.Count) artifacts to $registerPath"

