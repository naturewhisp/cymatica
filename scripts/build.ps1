<#
.SYNOPSIS
    Configure, build, test and run CYMATICA on Windows.
.DESCRIPTION
    Works from any PowerShell (including non-interactive agent shells): if the MSVC
    environment is not loaded, it enters the Visual Studio Developer Shell of the
    latest *stable* installation found by vswhere (Preview/Insiders channels are
    ignored because -prerelease is not passed). The environment change only
    affects this script's process.
.PARAMETER Config
    Release (default) or Debug.
.PARAMETER Test
    Run CTest after building.
.PARAMETER Headless
    With -Test, skip tests labelled "device" (they need audio hardware).
.PARAMETER Run
    Launch cymatica_game after building.
.PARAMETER SmokeSeconds
    With -Run, close the game automatically after this many seconds.
.PARAMETER Clean
    Delete the build directory of the selected preset first.
.NOTES
    MSVC toolset: ADR-0001 pins 14.50. Override with $env:CYMATICA_VCVARS_VER
    (set it to an empty string to use the installation default).
#>
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Config = 'Release',
    [switch]$Test,
    [switch]$Headless,
    [switch]$Run,
    [double]$SmokeSeconds = 0,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "'$Exe $($Arguments -join ' ')' failed with exit code $LASTEXITCODE" }
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found: install Visual Studio Build Tools (see docs/build.md)." }
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'No stable Visual Studio installation with the C++ x64 toolset was found.' }

    $toolset = if ($null -ne $env:CYMATICA_VCVARS_VER) { $env:CYMATICA_VCVARS_VER } else { '' }
    $devArgs = '-arch=x64 -host_arch=x64'
    if ($toolset) { $devArgs += " -vcvars_ver=$toolset" }

    Write-Host "[build.ps1] MSVC environment: $vsPath (toolset: $(if ($toolset) { $toolset } else { 'default' }))" -ForegroundColor Cyan
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments $devArgs | Out-Null
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw "MSVC toolset '$toolset' could not be activated." }
}

$preset = "x64-$($Config.ToLower())"
$buildDir = Join-Path $repoRoot "build\$preset"

Push-Location $repoRoot
try {
    if ($Clean -and (Test-Path $buildDir)) {
        Write-Host "[build.ps1] Removing $buildDir" -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
    }

    Invoke-Checked cmake @('--preset', $preset)
    Invoke-Checked cmake @('--build', '--preset', $preset)

    if ($Test) {
        $testPreset = if ($Headless) {
            if ($Config -ne 'Release') { throw '-Headless is only defined for Release (preset x64-release-headless).' }
            'x64-release-headless'
        } else { $preset }
        Invoke-Checked ctest @('--preset', $testPreset)
    }

    if ($Run) {
        $exe = Join-Path $buildDir 'apps\cymatica_game\cymatica_game.exe'
        $runArgs = @()
        if ($SmokeSeconds -gt 0) {
            $runArgs = @('--smoke-seconds', $SmokeSeconds.ToString([Globalization.CultureInfo]::InvariantCulture))
        }
        Invoke-Checked $exe $runArgs
    }
}
finally {
    Pop-Location
}
