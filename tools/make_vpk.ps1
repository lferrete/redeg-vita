#requires -Version 5.1
<#
  Builds redeg.vpk from the SELF produced by CMake.

  CMake's vita_create_vpk macro escapes spaces in the asset paths for MSYS
  make; this script invokes vita-pack-vpk with native Windows argument
  quoting. It packs only the loader, SFO, LiveArea art and provenance file.
#>
# Port code Copyright (C) 2026 Leandro Ferrete.
param(
    [string]$BuildDir = "",
    [string]$OutFile = "",
    [string]$ArtDir = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$vitasdk = if ($env:VITASDK) { $env:VITASDK } else { "C:\vitasdk" }
if ($vitasdk -match "^/c/") { $vitasdk = "C:\" + $vitasdk.Substring(3).Replace("/", "\") }
if (-not $BuildDir) { $BuildDir = Join-Path $projectRoot "build" }
if (-not $OutFile)  { $OutFile  = Join-Path $BuildDir "redeg.vpk" }

$eboot = Join-Path $BuildDir "eboot.bin"
if (-not (Test-Path -LiteralPath $eboot)) {
    throw "eboot.bin not found in $BuildDir. Build the target first."
}

$sfo = Join-Path $BuildDir "param.sfo"
& (Join-Path $vitasdk "bin\vita-mksfoex.exe") `
    -d "ATTRIBUTE2=12" `
    -s "APP_VER=01.00" `
    -s "TITLE_ID=REDEGEN01" `
    -s "PORT_AUTHOR=Leandro Ferrete" `
    -s "PORT_VERSION=1.0" `
    "Resident Evil: Degeneration" `
    $sfo

if (-not $ArtDir) {
    $ArtDir = Join-Path $projectRoot "extras\livearea"
} elseif (-not [System.IO.Path]::IsPathRooted($ArtDir)) {
    $ArtDir = Join-Path $projectRoot $ArtDir
}
$assets = $ArtDir
$provenance = Join-Path $projectRoot "extras\provenance.txt"
$template = Join-Path $assets "template.xml"

if (-not (Test-Path -LiteralPath $provenance)) {
    throw "Provenance file not found: $provenance"
}
if (-not (Test-Path -LiteralPath $template)) {
    throw "LiveArea template not found: $template"
}
if (Test-Path -LiteralPath $OutFile) {
    Remove-Item -Force -LiteralPath $OutFile
}

& (Join-Path $vitasdk "bin\vita-pack-vpk.exe") `
    -a "$assets\icon0.png=sce_sys/icon0.png" `
    -a "$assets\pic0.png=sce_sys/pic0.png" `
    -a "$assets\startup.png=sce_sys/livearea/contents/startup.png" `
    -a "$assets\bg0.png=sce_sys/livearea/contents/bg0.png" `
    -a "$template=sce_sys/livearea/contents/template.xml" `
    -a "$provenance=sce_sys/provenance.txt" `
    -s $sfo `
    -b $eboot `
    $OutFile

if (-not (Test-Path -LiteralPath $OutFile)) {
    throw "vita-pack-vpk did not create $OutFile"
}

$sha256 = [System.Security.Cryptography.SHA256]::Create()
$stream = [System.IO.File]::OpenRead($OutFile)
try {
    $hash = [System.BitConverter]::ToString($sha256.ComputeHash($stream)).Replace("-", "")
} finally {
    $stream.Dispose()
    $sha256.Dispose()
}
$sums = Join-Path $BuildDir "SHA256SUMS.txt"
@(
    "# Resident Evil: Degeneration - PS Vita port by Leandro Ferrete",
    ("{0}  {1}" -f $hash, (Split-Path -Leaf $OutFile))
) | Set-Content -LiteralPath $sums -Encoding ASCII

Write-Host ("VPK: {0} ({1:N0} bytes)" -f $OutFile, (Get-Item -LiteralPath $OutFile).Length) -ForegroundColor Green
Write-Host ("SHA256: {0}" -f $hash) -ForegroundColor Green
Write-Host ("Sums: {0}" -f $sums) -ForegroundColor Green
