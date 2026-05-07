$ErrorActionPreference = "Stop"

# Rocket Hideout B4F (0xCA), Lift Key grunt at (11,2) in current event data.
# Place player directly in front of him (one tile south): (11,3).
$mapId = "0xCA"
$x = 11
$y = 3

$cmdDir = Join-Path $PSScriptRoot "..\bugs"
$cmdPath = Join-Path $cmdDir "cli_cmd.txt"

New-Item -ItemType Directory -Force -Path $cmdDir | Out-Null
Set-Content -Path $cmdPath -Value ("teleport {0} {1} {2}" -f $mapId, $x, $y) -Encoding ascii

Write-Host ("Queued debug command: teleport {0} {1} {2}" -f $mapId, $x, $y)
Write-Host ("File: {0}" -f (Resolve-Path $cmdPath))
