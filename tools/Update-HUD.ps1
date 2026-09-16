#requires -Version 5.1
<#
.SYNOPSIS
  Rebuild and publish the Darkest Dungeon Reinforcement HUD after a game update.

.DESCRIPTION
  This script is the repeatable update pipeline for the HUD:
    1. Read the known-memory offsets for the current Darkest.exe build from tools\offsets.json.
    2. Patch src\GlHudNice.cpp with those offsets.
    3. Rebuild ReinforcementHudColor.dll.
    4. Copy the DLL into the Steam Workshop content folder.
    5. Rebuild the release zip.
    6. Optionally commit/push to GitHub and/or upload to Steam Workshop.

  If offsets for the new build are not yet in tools\offsets.json, first follow
  docs\OFFSET_UPDATE_GUIDE.md, add the new build entry, then run this script.

.PARAMETER BuildId
  Darkest Dungeon Steam buildid. If omitted, the script tries to read it from
  appmanifest_262060.acf in common Steam library folders.

.PARAMETER Compiler
  Path to x86_64-w64-mingw32-g++. If omitted, PATH and the winget LLVM-MinGW
  package location are searched.

.PARAMETER WorkshopDir
  Folder containing item.vdf and content\. Defaults to
  ..\DD_ReinforcementHUD_Workshop beside the repository.

.PARAMETER PushGit
  Commit and push the rebuilt source/DLL to GitHub.

.PARAMETER PublishWorkshop
  Run SteamCMD workshop_build_item for the existing Workshop item.

.PARAMETER SteamCmd
  Path to steamcmd.exe.

.PARAMETER SteamUser
  Steam account name used by SteamCMD.

.EXAMPLE
  .\tools\Update-HUD.ps1

.EXAMPLE
  .\tools\Update-HUD.ps1 -PushGit -PublishWorkshop
#>
[CmdletBinding()]
param(
    [string]$BuildId,
    [string]$Compiler,
    [string]$WorkshopDir,
    [switch]$PushGit,
    [switch]$PublishWorkshop,
    [string]$SteamCmd = 'D:\Workspaces\steamcmd\steamcmd.exe',
    [string]$SteamUser = 'shenby1999'
)

$ErrorActionPreference = 'Stop'

function Write-Step($text) {
    Write-Host ""
    Write-Host "==> $text" -ForegroundColor Cyan
}

function Get-ManifestBuildId([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    $text = Get-Content -Raw -LiteralPath $path
    $m = [regex]::Match($text, '"buildid"\s+"(\d+)"')
    if ($m.Success) { return $m.Groups[1].Value }
    return $null
}

function Find-SteamManifest {
    $roots = New-Object System.Collections.Generic.List[string]
    $common = @(
        'C:\Program Files (x86)\Steam',
        'C:\Program Files\Steam',
        'D:\Softwares\steam',
        'D:\Steam',
        'D:\SteamLibrary',
        'E:\Steam',
        'E:\SteamLibrary'
    )
    foreach ($r in $common) { if (Test-Path -LiteralPath $r) { $roots.Add($r) } }

    foreach ($root in $roots) {
        $manifest = Join-Path $root 'steamapps\appmanifest_262060.acf'
        if (Test-Path -LiteralPath $manifest) { return $manifest }

        $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf) {
            foreach ($line in Get-Content -LiteralPath $vdf) {
                $m = [regex]::Match($line, '"path"\s+"(.+?)"')
                if ($m.Success) {
                    $lib = $m.Groups[1].Value.Replace('\\', '\')
                    $candidate = Join-Path $lib 'steamapps\appmanifest_262060.acf'
                    if (Test-Path -LiteralPath $candidate) { return $candidate }
                }
            }
        }
    }
    return $null
}

function Find-Compiler {
    if ($Compiler) {
        if (Test-Path -LiteralPath $Compiler) { return $Compiler }
        throw "Compiler not found: $Compiler"
    }

    $cmd = Get-Command 'x86_64-w64-mingw32-g++' -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $base = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'
    if (Test-Path -LiteralPath $base) {
        $found = Get-ChildItem -LiteralPath $base -Directory -Filter 'MartinStorsjo.LLVM-MinGW.UCRT_*' -ErrorAction SilentlyContinue |
            ForEach-Object {
                Get-ChildItem -LiteralPath $_.FullName -Recurse -Filter 'x86_64-w64-mingw32-g++.exe' -ErrorAction SilentlyContinue
            } | Select-Object -First 1
        if ($found) { return $found.FullName }
    }

    throw "Cannot find x86_64-w64-mingw32-g++. Pass -Compiler <path>."
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$offsetsFile = Join-Path $PSScriptRoot 'offsets.json'
$sourceFile = Join-Path $repoRoot 'src\GlHudNice.cpp'
$dllFile = Join-Path $repoRoot 'ReinforcementHudColor.dll'

if (-not (Test-Path -LiteralPath $offsetsFile)) { throw "Missing offsets file: $offsetsFile" }
if (-not (Test-Path -LiteralPath $sourceFile)) { throw "Missing source file: $sourceFile" }

if (-not $BuildId) {
    Write-Step 'Resolving Darkest Dungeon buildid'
    $manifest = Find-SteamManifest
    if ($manifest) {
        $BuildId = Get-ManifestBuildId $manifest
        Write-Host "Manifest: $manifest"
    }
    if (-not $BuildId) {
        throw "Cannot auto-detect buildid. Pass -BuildId <buildid>, or make sure appmanifest_262060.acf exists in a known Steam folder."
    }
}

Write-Host "Target buildid: $BuildId"

$data = Get-Content -Raw -LiteralPath $offsetsFile | ConvertFrom-Json
$entry = $data.builds.$BuildId
if (-not $entry) {
    Write-Host ""
    Write-Host "No offsets entry for buildid $BuildId." -ForegroundColor Yellow
    Write-Host "Add it to tools\offsets.json after following docs\OFFSET_UPDATE_GUIDE.md." -ForegroundColor Yellow
    exit 2
}

Write-Step 'Patching src\GlHudNice.cpp'
$src = Get-Content -Raw -LiteralPath $sourceFile
$src = [regex]::Replace($src, '(?m)^// Darkest Dungeon 1 build .*$', "// Darkest Dungeon 1 build $BuildId offsets.")
$src = [regex]::Replace($src,
    'uintptr_t app = \*\(uintptr_t\*\)\(base \+ 0x[0-9A-Fa-f]+\);',
    "uintptr_t app = *(uintptr_t*)(base + $($entry.appStatePtrRVA));")
$src = [regex]::Replace($src,
    'count = \*\(int\*\)\(app \+ 0x[0-9A-Fa-f]+\); flag = \*\(unsigned char\*\)\(app \+ 0x[0-9A-Fa-f]+\); summon = \*\(int\*\)\(base \+ 0x[0-9A-Fa-f]+\);',
    "count = *(int*)(app + $($entry.stallCountOffset)); flag = *(unsigned char*)(app + $($entry.stallFlagOffset)); summon = *(int*)(base + $($entry.summonThresholdRVA));")
[System.IO.File]::WriteAllText($sourceFile, $src, (New-Object System.Text.UTF8Encoding($false)))

Write-Step 'Building ReinforcementHudColor.dll'
$gxx = Find-Compiler
Write-Host "Compiler: $gxx"
Push-Location $repoRoot
try {
    & $gxx '-shared' '-O2' '-static' '-static-libgcc' '-static-libstdc++' 'src\GlHudNice.cpp' '-o' 'ReinforcementHudColor.dll' '-lopengl32'
    if ($LASTEXITCODE -ne 0) { throw "Compiler failed with exit code $LASTEXITCODE" }
}
finally {
    Pop-Location
}
if (-not (Test-Path -LiteralPath $dllFile)) { throw "Build did not produce $dllFile" }
Write-Host "Built: $dllFile"

if (-not $WorkshopDir) {
    $WorkshopDir = Join-Path (Split-Path $repoRoot -Parent) 'DD_ReinforcementHUD_Workshop'
}

if (Test-Path -LiteralPath $WorkshopDir) {
    $workshopContent = Join-Path $WorkshopDir 'content'
    if (-not (Test-Path -LiteralPath $workshopContent)) { New-Item -ItemType Directory -Path $workshopContent | Out-Null }
    Copy-Item -LiteralPath $dllFile -Destination (Join-Path $workshopContent 'ReinforcementHudColor.dll') -Force
    Write-Host "Updated workshop content: $workshopContent"
} else {
    Write-Host "Workshop folder not found, skipped: $WorkshopDir" -ForegroundColor Yellow
}

Write-Step 'Rebuilding release zip'
$releaseZip = Join-Path (Split-Path $repoRoot -Parent) 'DD_ReinforcementHUD_Release_v1.0.0.zip'
$releaseFiles = @(
    (Join-Path $repoRoot 'StartWithHud.exe'),
    (Join-Path $repoRoot 'Injector.exe'),
    (Join-Path $repoRoot 'ReinforcementHudColor.dll'),
    (Join-Path $repoRoot 'StartHUD.bat'),
    (Join-Path $repoRoot 'README.md')
)
if (Test-Path -LiteralPath $releaseZip) { Remove-Item -LiteralPath $releaseZip -Force }
Compress-Archive -LiteralPath $releaseFiles -DestinationPath $releaseZip -CompressionLevel Optimal
Write-Host "Release zip: $releaseZip"

if ($PushGit) {
    Write-Step 'Committing and pushing to GitHub'
    Push-Location $repoRoot
    try {
        git add src\GlHudNice.cpp ReinforcementHudColor.dll docs\REVERSE_ENGINEERING.md docs\OFFSET_UPDATE_GUIDE.md tools\offsets.json tools\Update-HUD.ps1 README.md
        $status = git status --porcelain
        if ($status) {
            git commit -m "Update HUD offsets for build $BuildId"
            git push origin master
        } else {
            Write-Host "No source changes to commit."
        }
    }
    finally {
        Pop-Location
    }
}

if ($PublishWorkshop) {
    Write-Step 'Publishing to Steam Workshop'
    if (-not (Test-Path -LiteralPath $SteamCmd)) { throw "SteamCMD not found: $SteamCmd" }
    $itemVdf = Join-Path $WorkshopDir 'item.vdf'
    if (-not (Test-Path -LiteralPath $itemVdf)) { throw "item.vdf not found: $itemVdf" }
    & $SteamCmd '+login' $SteamUser '+workshop_build_item' $itemVdf '+quit'
    if ($LASTEXITCODE -ne 0) { throw "SteamCMD failed with exit code $LASTEXITCODE" }
}

Write-Step 'Done'
Write-Host "Build:            $BuildId"
Write-Host "DLL:              $dllFile"
Write-Host "Workshop content: $(Join-Path $WorkshopDir 'content')"
Write-Host "Release zip:      $releaseZip"
