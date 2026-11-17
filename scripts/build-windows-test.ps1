# Run from Visual Studio 2022 Developer PowerShell (x64).
[CmdletBinding()]
param(
    [string]$JucePath,
    [switch]$FetchPinnedJuce,
    [string]$BuildDirectory,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'A real Windows MSVC environment is required.' }
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $repoRoot 'build/windows-alpha' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repoRoot 'dist/windows-alpha' }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Output already exists. Choose a fresh -OutputDirectory; nothing was removed.' }
foreach ($tool in @('git','node','cmake','cl.exe')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing $tool. Open VS2022 x64 Developer PowerShell with C++ desktop tools, Git, Node20+ and CMake3.22+." }
}
if ($JucePath -and $FetchPinnedJuce) { throw 'Use -JucePath or -FetchPinnedJuce, not both.' }
if (-not $JucePath -and -not $FetchPinnedJuce) { throw 'Provide a clean pinned JUCE8.0.8 checkout via -JucePath, or explicitly allow its download with -FetchPinnedJuce.' }
function Invoke-Checked {
    param([string]$File, [string[]]$CommandArgs)
    & $File @CommandArgs
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE. No successful release is claimed." }
}
Push-Location $repoRoot
try {
    $changed = & git diff HEAD --name-only
    if ($LASTEXITCODE -ne 0 -or $changed) { throw 'Commit tracked source changes first: packaged sources must match HEAD.' }
    Invoke-Checked 'node' @('--version')
    Invoke-Checked 'cmake' @('--version')
    Invoke-Checked 'npm.cmd' @('test')
    $configureArgs = @('-S','plugin','-B',$BuildDirectory,'-G','Visual Studio 17 2022','-A','x64')
    if ($JucePath) { $configureArgs += "-DLOOPFORGE_JUCE_PATH=$JucePath" }
    Invoke-Checked 'cmake' $configureArgs
    Invoke-Checked 'cmake' @('--build',$BuildDirectory,'--config','Release','--parallel','2')
    Invoke-Checked 'ctest' @('--test-dir',$BuildDirectory,'-C','Release','--output-on-failure')
    $executables = Join-Path $BuildDirectory 'Release'
    Invoke-Checked 'node' @('scripts/verify-native-api.cjs',(Join-Path $executables 'processor_test.exe'))
    Invoke-Checked 'node' @('scripts/verify-native-failures.cjs',(Join-Path $executables 'processor_test.exe'))
    Invoke-Checked 'node' @('scripts/verify-composer-worker.cjs',(Join-Path $executables 'composer_processor_test.exe'))
    if (-not $JucePath) { $JucePath = Join-Path $BuildDirectory '_deps/juce-src' }
    Invoke-Checked 'node' @('scripts/package-native.cjs','--build-dir',$BuildDirectory,'--juce-dir',$JucePath,'--output',$OutputDirectory,'--platform','windows')
    Write-Host "Native Windows EXE + VST3 candidate and SHA256SUMS: $OutputDirectory"
    Write-Host 'Compilation/tests do not certify FL Studio or real audio-device operation. Follow FL-STUDIO-TEST.md.'
} finally { Pop-Location }
