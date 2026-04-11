# make-builder-yaml.ps1
# Generates self-contained *-builder.yaml files by inlining packages into a single file.
# Suitable for the ESPHome web builder or dashboard where relative !include paths don't work.
#
# Usage:
#   .\make-builder-yaml.ps1                        # process all standard device yamls
#   .\make-builder-yaml.ps1 -Target .\levoit-core300s\levoit-core300s.yaml

param(
    [string]$Target = ""   # single yaml to process; leave empty to process all
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot

$SourceGitContent = @"
external_components:
  - source:
      type: git
      url: https://github.com/tuct/levoit
      ref: main
    components: [levoit]
"@

function Make-Builder {
    param([string]$DeviceYaml)

    $deviceDir  = Split-Path $DeviceYaml -Parent
    $deviceName = [System.IO.Path]::GetFileNameWithoutExtension($DeviceYaml)
    $commonYaml = Join-Path $deviceDir "common.yaml"

    # naming: {base}-builder{-suffix}.yaml
    # e.g. levoit-core400s-c3.yaml  -> levoit-core400s-builder-c3.yaml
    #      levoit-core400s.yaml     -> levoit-core400s-builder.yaml
    $folderBase = Split-Path $deviceDir -Leaf   # e.g. "levoit-core400s"
    $suffix     = $deviceName.Substring($folderBase.Length)  # e.g. "-c3" or ""
    $outputFile = Join-Path $deviceDir "$folderBase-builder$suffix.yaml"

    if (-not (Test-Path $commonYaml)) {
        Write-Host "  SKIP (no common.yaml): $DeviceYaml" -ForegroundColor Yellow
        return
    }

    # --- device yaml: everything before the packages: block ---
    $deviceRaw = Get-Content $DeviceYaml -Raw
    $packagesIdx = $deviceRaw.IndexOf("`npackages:")
    if ($packagesIdx -lt 0) {
        Write-Host "  SKIP (no packages: block): $DeviceYaml" -ForegroundColor Yellow
        return
    }
    $deviceHeader = $deviceRaw.Substring(0, $packagesIdx).TrimEnd()

    # --- common.yaml: skip the first comment line ---
    $commonLines = Get-Content $commonYaml
    $firstContent = 0
    for ($i = 0; $i -lt $commonLines.Count; $i++) {
        if ($commonLines[$i] -match "^\s*#") { $firstContent = $i + 1 } else { break }
    }
    $commonBody = ($commonLines[$firstContent..($commonLines.Count - 1)] -join "`n").Trim()

    # --- assemble ---
    $output = @"
$deviceHeader

$SourceGitContent
$commonBody
"@

    $verb = if (Test-Path $outputFile) { "Updated" } else { "Created" }
    Set-Content -Path $outputFile -Value $output -Encoding UTF8 -Force
    Write-Host "  $verb`: $outputFile" -ForegroundColor Green
}

# --- collect targets ---
if ($Target -ne "") {
    $files = @(Resolve-Path $Target)
} else {
    # all non-dev, non-builder device yamls that have a packages: block
    $files = Get-ChildItem -Path $ScriptDir -Recurse -Filter "*.yaml" |
        Where-Object {
            $_.Name -notmatch "_dev\.|_builder\.|^common|^secrets|^packages" -and
            $_.FullName -notmatch "\\packages\\" -and
            (Get-Content $_.FullName -Raw) -match "`npackages:"
        }
}

Write-Host "`nGenerating builder YAMLs...`n"
foreach ($f in $files) {
    Make-Builder -DeviceYaml $f.FullName
}
Write-Host "`nDone."
