#requires -Version 5.1
<#
  Resident Evil: Degeneration - PS Vita data preparation

  Extracts Payload/residentevil.app/ from a legally obtained, decrypted iOS
  IPA into .\data\game\, ready to be copied to ux0:data/redeg/ on the Vita.

  The script never modifies the IPA. It does not download anything.

  Usage:
    powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1
    powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 -Ipa path\to\game.ipa
    powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 -AppDir path\to\residentevil.app
#>
# Port code Copyright (C) 2026 Leandro Ferrete.
param(
    [string]$Ipa = "",
    [string]$AppDir = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression.FileSystem

$projectRoot = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $projectRoot
if (-not $OutDir) { $OutDir = Join-Path $projectRoot "data" }

if (-not $Ipa -and -not $AppDir) {
    $ipaFile = Get-ChildItem -LiteralPath $root -Filter "*.ipa" -File | Select-Object -First 1
    if ($ipaFile) {
        $Ipa = $ipaFile.FullName
    } else {
        throw "IPA not found. Pass -Ipa or -AppDir explicitly."
    }
}

if ($Ipa -and $AppDir) {
    throw "Pass only one of -Ipa or -AppDir."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$gameDir = Join-Path $OutDir "game"
New-Item -ItemType Directory -Force -Path $gameDir | Out-Null
foreach ($d in @("logs", "shader_cache", "game\Documents")) {
    New-Item -ItemType Directory -Force -Path (Join-Path $OutDir $d) | Out-Null
}

function Copy-AppContents([string]$SourceDir) {
    if (-not (Test-Path -LiteralPath $SourceDir)) {
        throw "App directory not found: $SourceDir"
    }
    Copy-Item -Path (Join-Path $SourceDir "*") -Destination $gameDir -Recurse -Force
    Write-Host "  Copied app contents from $SourceDir"
}

if ($AppDir) {
    Write-Host "AppDir: $AppDir"
    Copy-AppContents $AppDir
} else {
    Write-Host "IPA:    $Ipa"
    $ipaPath = (Resolve-Path -LiteralPath $Ipa).Path
    $zip = [System.IO.Compression.ZipFile]::OpenRead($ipaPath)
    try {
        $prefix = "Payload/residentevil.app/"
        $entries = @($zip.Entries | Where-Object {
            $_.FullName.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase) -and
            -not $_.FullName.EndsWith("/")
        })
        if ($entries.Count -eq 0) {
            throw "Payload/residentevil.app/ not found in the IPA. Is this the correct game?"
        }

        foreach ($entry in $entries) {
            $relative = $entry.FullName.Substring($prefix.Length).Replace("/", "\")
            $dst = Join-Path $gameDir $relative
            $parent = Split-Path -Parent $dst
            if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dst, $true)
            Write-Host ("  {0} -> game\{1} ({2:N0} bytes)" -f $entry.FullName, $relative, $entry.Length)
        }

        # The original key art is useful for regenerating LiveArea art. It is
        # ignored by git and never enters the VPK.
        $art = $zip.GetEntry("iTunesArtwork")
        if ($art) {
            $artDst = Join-Path $projectRoot "extras\livearea\source_icon.png"
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artDst) | Out-Null
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($art, $artDst, $true)
            Write-Host "  iTunesArtwork -> extras\livearea\source_icon.png (ignored by git)"
        }
    } finally {
        $zip.Dispose()
    }
}

$required = @("residentevil", "main.dz", "s3e.icf", "app.icf", "MainWindow.nib")
$missing = @()
foreach ($name in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $gameDir $name))) {
        $missing += $name
    }
}

@"
Resident Evil: Degeneration - PS Vita data
==========================================

Copy the contents of this folder to:

    ux0:data/redeg/

Required final layout:

    ux0:data/redeg/
      game/
        residentevil
        main.dz
        s3e.icf
        app.icf
        MainWindow.nib
        intro.mp4
        Info.plist
        PkgInfo
        Default.png
        Icon.png
        Documents/
      logs/
      shader_cache/

Do NOT extract or rename main.dz. The original game engine reads it as a
DZip container.

The application does not include or redistribute the original game code or
assets. You must own a legal, decrypted copy of the iOS release.
"@ | Set-Content -LiteralPath (Join-Path $OutDir "README.txt") -Encoding utf8

if ($missing.Count -gt 0) {
    Write-Warning ("Missing required file(s): " + ($missing -join ", "))
    Write-Warning "The port will not boot until those files are present."
} else {
    Write-Host "  All required game files are present." -ForegroundColor Green
}

Write-Host ""
Write-Host "Done. Copy '$OutDir' to 'ux0:data/redeg' on the Vita." -ForegroundColor Green
