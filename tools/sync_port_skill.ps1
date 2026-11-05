# Sync the android-to-vita-port skill to:
#   1. the active skills directory (~/.agents/skills)
#   2. the OpenCode global skills directory (~/.config/opencode/skills)
#   3. the workspace-root .opencode/skills
#
# The versioned source is skills/android-to-vita-port in this repository.
# Copies are merged without deleting unrelated references in destinations.
# Refresh the RE Degeneration case study from this repository before copying.
# Run from anywhere:
#   powershell -ExecutionPolicy Bypass -File tools\sync_port_skill.ps1
[CmdletBinding()]
param(
    [string]$Source = '',
    [string]$ActiveDest = '',
    [string]$GlobalDest = '',
    [string]$RootDest = ''
)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Source) {
    $Source = Join-Path $ScriptDir '..\skills\android-to-vita-port'
}
if (-not $ActiveDest) {
    $ActiveDest = Join-Path $env:USERPROFILE '.agents\skills\android-to-vita-port'
}
if (-not $GlobalDest) {
    $GlobalDest = Join-Path $env:USERPROFILE '.config\opencode\skills\android-to-vita-port'
}
if (-not $RootDest) {
    $RootDest = Join-Path $ScriptDir '..\..\.opencode\skills\android-to-vita-port'
}
$Source = (Resolve-Path -LiteralPath $Source).Path

if (-not (Test-Path -LiteralPath (Join-Path $Source 'SKILL.md'))) {
    throw "SKILL.md not found under $Source"
}

$caseStudy = Join-Path $ScriptDir '..\docs\GRAPHICS-INVESTIGATION-2026-11-05.md'
if (Test-Path -LiteralPath $caseStudy) {
    $references = Join-Path $Source 'references'
    New-Item -ItemType Directory -Force -Path $references | Out-Null
    Copy-Item -LiteralPath $caseStudy -Destination (Join-Path $references '17-redeg-graphics-investigation.md') -Force
}

foreach ($dest in @($ActiveDest, $GlobalDest, $RootDest)) {
    $dest = [System.IO.Path]::GetFullPath($dest)
    if ($dest.TrimEnd('\', '/') -ieq $Source.TrimEnd('\', '/')) {
        continue
    }
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item -Path (Join-Path $Source '*') -Destination $dest -Recurse -Force
    Write-Host "synced -> $dest"
}

Write-Host "done: $Source"
