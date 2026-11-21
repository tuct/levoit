# Run the CI compile tests locally (Windows). macOS / Linux: run-compile-tests.sh
#
#   .\components\run-compile-tests.ps1                          # ESPHome on your PATH
#   .\components\run-compile-tests.ps1 min                      # minimum version from the CI workflow
#   .\components\run-compile-tests.ps1 2026.10.0                # any other version
#   .\components\run-compile-tests.ps1 min -Component levoit    # only some components
#
# If scripts are blocked:  powershell -ExecutionPolicy Bypass -File components\run-compile-tests.ps1
#
# A pinned version is installed once into .venv-esphome-<version>\ at the repo
# root (git-ignored), so your normal ESPHome install is not touched.
# Full build logs: components\<component>\tests\.esphome\<config>.log
param(
    [string]$Esphome = "",
    [string[]]$Component = @("levoit", "philips")
)

$Root = Split-Path -Parent $PSScriptRoot
$Workflow = Join-Path $Root ".github\workflows\compile-tests.yml"
# Every components\<component>\tests\test-*.yaml is built, so a new test
# config (e.g. one that only exists for levoit) needs no change here.

if ($Esphome -eq "min") {
    $line = Select-String -Path $Workflow -Pattern '^\s*esphome:\s*\[' | Select-Object -First 1
    $m = [regex]::Match([string]$line, '\d{4}\.\d+\.\d+')
    if (-not $m.Success) { Write-Host "Could not read the minimum ESPHome version from $Workflow"; exit 1 }
    $Esphome = $m.Value
}

if ($Esphome -and $Esphome -ne "latest") {
    $venv = Join-Path $Root ".venv-esphome-$Esphome"
    $exe = Join-Path $venv "Scripts\esphome.exe"
    if (-not (Test-Path $exe)) {
        Write-Host "Installing ESPHome $Esphome into $venv ..."
        $py = "python"
        if (-not (Get-Command $py -ErrorAction SilentlyContinue)) { $py = "py" }
        & $py -m venv $venv
        & (Join-Path $venv "Scripts\python.exe") -m pip install --quiet "esphome==$Esphome"
        if ($LASTEXITCODE -ne 0) { Write-Host "pip install failed"; exit 1 }
    }
} else {
    $exe = "esphome"
}

$version = [string](& $exe version)
Write-Host "Using $version"
$failed = 0
foreach ($c in $Component) {
    $dir = Join-Path $Root "components\$c\tests"
    $esphomeDir = Join-Path $dir ".esphome"
    New-Item -ItemType Directory -Force $esphomeDir | Out-Null
    # A build folder left by another ESPHome version breaks the link (stale
    # generated files), so start clean whenever the version changes.
    $marker = Join-Path $esphomeDir "esphome-version"
    $build = Join-Path $esphomeDir "build"
    $last = ""
    if (Test-Path $marker) { $last = (Get-Content $marker -Raw).Trim() }
    if ($last -ne $version.Trim() -and (Test-Path $build)) {
        Write-Host "$c`: ESPHome version changed ($last -> $version), removing generated $build"
        Remove-Item -Recurse -Force $build -Confirm:$false
    }
    Set-Content -Path $marker -Value $version.Trim() -Encoding ASCII
    $configs = Get-ChildItem -Path $dir -Filter "test-*.yaml" | Sort-Object Name | ForEach-Object { $_.BaseName }
    foreach ($t in $configs) {
        $log = Join-Path $esphomeDir "$t.log"
        Push-Location $dir
        & $exe compile "$t.yaml" 2>&1 | Out-File -Encoding utf8 $log
        $code = $LASTEXITCODE
        Pop-Location
        if ($code -eq 0) {
            Write-Host ("{0,-8} {1,-24} PASS" -f $c, $t)
        } else {
            Write-Host ("{0,-8} {1,-24} FAIL  (log: {2})" -f $c, $t, $log)
            Select-String -Path $log -Pattern "error:" | Select-Object -First 5 | ForEach-Object { Write-Host ("    " + $_.Line) }
            $failed++
        }
    }
}

if ($failed -eq 0) { Write-Host "All passed." } else { Write-Host "$failed failed." }
exit $failed
