# make-builder-yaml.ps1
# Generates the self-contained *-builder*.yaml files: one file per device config,
# with its packages inlined and the component pulled from GitHub. Paste one into
# the ESPHome Device Builder, where relative !include and local paths don't work.
#
# Usage (Windows PowerShell 5.1 or PowerShell 7, from any folder):
#   .\make-builder-yaml.ps1                        # regenerate all
#   .\make-builder-yaml.ps1 -Target .\levoit-core300s\levoit-core300s.yaml
#   .\make-builder-yaml.ps1 -Check                 # CI: exit 1 if a builder file is out of date
#
# Naming:
#   levoit-core300s.yaml     -> levoit-core300s-builder.yaml
#   levoit-core300s-c3.yaml  -> levoit-core300s-builder-c3.yaml
#   philips-ac0650.yaml      -> philips-ac0650-builder.yaml
#
# The component comes from the `stable` branch, which the Release workflow moves
# to every release, with `refresh: always`: ESPHome otherwise fetches a git
# component once a day, so a build right after a release (an update from the
# Levoit ESPHome integration) would still use the old commit.
# A device that needs other local components (the Sprout's
# levoit_audio) cannot be built in the Device Builder: it is skipped, and its
# builder file, if any, is kept by hand.
#
# Output is UTF-8 without BOM with LF line endings, the same on Windows and
# Linux, so -Check gives the same answer locally and in CI.
#
# It also writes devices/templates.json: the builder files the Levoit ESPHome
# integration offers for new devices (variant original, xiao_c3 or c3; S3
# builder files are not offered), with model, title, board and default name.

param(
    [string]$Target = "",   # single device yaml to process; empty = all
    [switch]$Check,         # compare instead of writing
    [string]$Ref = "stable" # git ref the builder files pull the component from
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$Utf8NoBom = New-Object System.Text.UTF8Encoding $false
# Components that live in this repo and can come from GitHub.
$RepoComponents = @("levoit", "philips")

function Read-Text([string]$Path) {
    # ReadAllText honours a BOM and defaults to UTF-8. Get-Content in
    # PowerShell 5.1 reads UTF-8 without a BOM as ANSI and garbles "—".
    return [System.IO.File]::ReadAllText($Path).Replace("`r`n", "`n")
}

function Split-ExternalComponents([string]$Text) {
    # Removes the top-level external_components: block(s) and returns the rest
    # plus the component names they listed.
    $keep = New-Object System.Collections.Generic.List[string]
    $components = New-Object System.Collections.Generic.List[string]
    $inBlock = $false
    foreach ($line in ($Text -split "`n")) {
        if ($inBlock) {
            if ($line -match '^\S') {
                $inBlock = $false   # the next top-level key ends the block
            } else {
                if ($line -match '^\s+components:\s*\[([^\]]*)\]') {
                    foreach ($c in $Matches[1].Split(',')) {
                        $c = $c.Trim().Trim('"', "'")
                        if ($c -ne "") { $components.Add($c) }
                    }
                } elseif ($line -match '^\s+components:\s*$') {
                    throw "external_components with a multi-line components list is not supported; use components: [a, b]"
                }
                continue
            }
        }
        if ($line -match '^external_components:\s*$') { $inBlock = $true; continue }
        $keep.Add($line)
    }
    return @{ Body = ($keep -join "`n"); Components = @($components) }
}

function Remove-LeadingComments([string]$Text) {
    $lines = $Text -split "`n"
    $first = 0
    while ($first -lt $lines.Count -and ($lines[$first] -match '^\s*#' -or $lines[$first].Trim() -eq "")) {
        $first++
    }
    if ($first -ge $lines.Count) { return "" }
    return ($lines[$first..($lines.Count - 1)] -join "`n").Trim()
}

function Get-OutputPath([string]$DeviceYaml) {
    $deviceDir  = Split-Path $DeviceYaml -Parent
    $deviceName = [System.IO.Path]::GetFileNameWithoutExtension($DeviceYaml)
    $folderBase = Split-Path $deviceDir -Leaf
    if ($deviceName.StartsWith($folderBase)) {
        # levoit-core400s-c3 in levoit-core400s/ -> levoit-core400s-builder-c3
        $suffix = $deviceName.Substring($folderBase.Length)
        return Join-Path $deviceDir "$folderBase-builder$suffix.yaml"
    }
    # philips-ac0650 in philips-600-series/ -> philips-ac0650-builder
    return Join-Path $deviceDir "$deviceName-builder.yaml"
}

function New-BuilderYaml([string]$DeviceYaml) {
    # Returns the builder file text, or $null (with a message) when skipped.
    $deviceDir = Split-Path $DeviceYaml -Parent
    $deviceRaw = Read-Text $DeviceYaml
    $m = [regex]::Match($deviceRaw, '(?m)^packages:[ \t]*$')
    if (-not $m.Success) {
        Write-Host "  SKIP (no packages: block): $DeviceYaml" -ForegroundColor Yellow
        return $null
    }

    $header = Split-ExternalComponents $deviceRaw.Substring(0, $m.Index).TrimEnd()
    $components = New-Object System.Collections.Generic.List[string]
    foreach ($c in $header.Components) { $components.Add($c) }

    # packages:   name: !include path   (inline real content; a package that only
    # holds external_components just contributes its component names)
    $bodies = New-Object System.Collections.Generic.List[string]
    $after = $deviceRaw.Substring($m.Index + $m.Length) -split "`n"
    foreach ($line in $after) {
        if ($line -match '^\S') { throw "content after the packages: block is not supported: $DeviceYaml" }
        if ($line.Trim() -eq "" -or $line -match '^\s*#') { continue }
        if ($line -notmatch '^\s+[\w-]+:\s*!include\s+(\S+)\s*$') {
            throw "unsupported packages entry in ${DeviceYaml}: $line"
        }
        $path = Join-Path $deviceDir $Matches[1]
        $package = Split-ExternalComponents (Read-Text $path)
        foreach ($c in $package.Components) { $components.Add($c) }
        $body = Remove-LeadingComments $package.Body
        if ($body -ne "") { $bodies.Add($body) }
    }

    $components = @($components | Select-Object -Unique)
    $foreign = @($components | Where-Object { $RepoComponents -notcontains $_ })
    if ($foreign.Count -gt 0) {
        Write-Host "  SKIP (needs local components: $($foreign -join ', ')): $DeviceYaml" -ForegroundColor Yellow
        return $null
    }
    if ($components.Count -eq 0) {
        Write-Host "  SKIP (no component from this repo): $DeviceYaml" -ForegroundColor Yellow
        return $null
    }

    # ESPHome merges a top-level key that the device yaml and a package both
    # set; a single inlined file cannot, it would have the key twice.
    $seen = @{}
    foreach ($part in @($header.Body) + @($bodies)) {
        $keys = @([regex]::Matches($part, '(?m)^([A-Za-z_][\w-]*):') | ForEach-Object { $_.Groups[1].Value })
        foreach ($key in $keys) {
            if ($seen.ContainsKey($key)) {
                throw "'${key}:' is set both in $DeviceYaml and in one of its packages. ESPHome merges that, a builder file cannot: put it in one place."
            }
        }
        foreach ($key in $keys) { $seen[$key] = $true }
    }

    $relative = (Split-Path $deviceDir -Leaf) + "/" + (Split-Path $DeviceYaml -Leaf)
    $source = @(
        "external_components:",
        "  - source:",
        "      type: git",
        "      url: https://github.com/tuct/levoit",
        "      ref: $Ref",
        "    components: [$($components -join ', ')]",
        "    refresh: always"
    ) -join "`n"

    return "# Generated from devices/$relative by devices/make-builder-yaml.ps1 - in the`n" +
           "# repo, edit that file or its common.yaml and regenerate, not this one.`n" +
           $header.Body.Trim() + "`n`n" + $source + "`n" + ($bodies -join "`n`n") + "`n"
}

# Titles shown by the Levoit ESPHome integration, per device folder.
$Titles = @{
    "levoit-core200s"       = "Levoit Core 200S"
    "levoit-core300s"       = "Levoit Core 300S"
    "levoit-core400s"       = "Levoit Core 400S"
    "levoit-core600s"       = "Levoit Core 600S"
    "levoit-eleva-300x"     = "Levoit Eleva 300X"
    "levoit-everest-air"    = "Levoit Everest Air"
    "levoit-superior-6000s" = "Levoit Superior 6000S"
    "levoit-vital100s"      = "Levoit Vital 100S"
    "levoit-vital200s"      = "Levoit Vital 200S"
    "philips-600-series"    = "Philips 600 series"
    "philips-900-series"    = "Philips 900 series"
}

function Get-TemplateEntry([string]$DeviceYaml, [string]$OutputPath, [string]$Text) {
    # A builder file the integration offers for new devices, or $null.
    $folder = Split-Path (Split-Path $OutputPath -Parent) -Leaf
    $file = Split-Path $OutputPath -Leaf
    if ($file -match '-builder-s3\.yaml$') { return $null }
    $board = ([regex]::Match($Text, '(?m)^  board:\s*(\S+)')).Groups[1].Value
    $name = ([regex]::Match($Text, '(?m)^  device_name:\s*(\S+)')).Groups[1].Value
    $m = [regex]::Match($Text, '(?m)^(levoit|philips):[ \t]*\n(?:[ \t]+.*\n)*?[ \t]+model:\s*(\S+)')
    if (-not $m.Success) { return $null }
    $component = $m.Groups[1].Value
    $model = $m.Groups[2].Value
    if ($model -match '^\$\{(\w+)\}$') {
        $model = ([regex]::Match($Text, "(?m)^  $($Matches[1]):\s*(\S+)")).Groups[1].Value
    }
    if ($board -eq "seeed_xiao_esp32c3") {
        $variant = "xiao_c3"
    } elseif ($component -eq "levoit" -and $file -eq "$folder-builder.yaml") {
        $variant = "original"
    } elseif ($board -match 'c3') {
        $variant = "c3"
    } else {
        return $null
    }
    $title = $Titles[$folder]
    if (-not $title) { $title = $folder }
    $source = "devices/" + $folder + "/" + (Split-Path $DeviceYaml -Leaf)
    return [pscustomobject]@{
        title = $title; model = $model; variant = $variant; board = $board
        name = $name; file = "devices/$folder/$file"; source = $source
    }
}

function ConvertTo-TemplatesJson($Entries) {
    # Written by hand, not ConvertTo-Json: the same bytes on PowerShell 5.1 and 7.
    $sorted = @($Entries | Sort-Object title, variant, file)
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("{")
    $lines.Add('  "version": 1,')
    $lines.Add('  "templates": [')
    for ($i = 0; $i -lt $sorted.Count; $i++) {
        $e = $sorted[$i]
        $fields = @("title", "model", "variant", "board", "name", "file", "source") |
            ForEach-Object {
                # no JSON escaping below: refuse values that would need it
                $value = [string]$e.$_
                if ($value.Contains('"') -or $value.Contains([string][char]92)) {
                    throw "templates.json: unexpected character in $($_): $value"
                }
                '"' + $_ + '": "' + $e.$_ + '"'
            }
        $comma = ","
        if ($i -eq $sorted.Count - 1) { $comma = "" }
        $lines.Add("    {" + ($fields -join ", ") + "}" + $comma)
    }
    $lines.Add("  ]")
    $lines.Add("}")
    return ($lines -join "`n") + "`n"
}

# --- collect targets ---
if ($Target -ne "") {
    $files = @((Resolve-Path $Target).Path)
} else {
    # every device yaml with a packages: block, except dev and builder files
    $files = @(Get-ChildItem -Path $ScriptDir -Recurse -Filter "*.yaml" |
        Where-Object {
            $_.Name -notmatch '_dev\.|builder|^common|^secrets' -and
            $_.Directory.Name -ne "packages" -and
            $_.FullName -notmatch '[\\/]\.esphome[\\/]' -and
            (Read-Text $_.FullName) -match '(?m)^packages:'
        } |
        Sort-Object FullName |
        ForEach-Object { $_.FullName })
}

$stale = New-Object System.Collections.Generic.List[string]
$templates = New-Object System.Collections.Generic.List[object]
Write-Host ""
foreach ($f in $files) {
    $text = New-BuilderYaml $f
    if ($null -eq $text) { continue }
    $out = Get-OutputPath $f
    if (($entry = Get-TemplateEntry $f $out $text) -ne $null) { $templates.Add($entry) }
    $exists = Test-Path $out
    $current = $null
    if ($exists) { $current = (Read-Text $out) }
    if ($current -eq $text) {
        Write-Host "  up to date: $out"
        continue
    }
    if ($Check) {
        $stale.Add($out)
        Write-Host "  OUT OF DATE: $out" -ForegroundColor Red
    } else {
        [System.IO.File]::WriteAllText($out, $text, $Utf8NoBom)
        $verb = "Updated"
        if (-not $exists) { $verb = "Created" }
        Write-Host "  $verb`: $out" -ForegroundColor Green
    }
}

if ($Target -eq "") {
    $json = ConvertTo-TemplatesJson $templates
    $index = Join-Path $ScriptDir "templates.json"
    $currentIndex = $null
    if (Test-Path $index) { $currentIndex = Read-Text $index }
    if ($currentIndex -ne $json) {
        if ($Check) {
            $stale.Add($index)
            Write-Host "  OUT OF DATE: $index" -ForegroundColor Red
        } else {
            [System.IO.File]::WriteAllText($index, $json, $Utf8NoBom)
            Write-Host "  Updated: $index" -ForegroundColor Green
        }
    } else {
        Write-Host "  up to date: $index"
    }
}

if ($Check -and $stale.Count -gt 0) {
    Write-Host "`n$($stale.Count) builder file(s) do not match their device yaml / common.yaml."
    Write-Host "Run devices/make-builder-yaml.ps1 and commit the result."
    exit 1
}
Write-Host "`nDone."
