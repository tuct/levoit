$ErrorActionPreference = "Stop"

$configs = @(
    ".\levoit-core200s\levoit-core200s_dev.yaml",
    ".\levoit-core300s\levoit-core300s_dev.yaml",
    ".\levoit-core400s\levoit-core400s_dev.yaml",
    ".\levoit-core600s\levoit-core600s_dev.yaml",
    ".\levoit-vital100s\levoit-vital100s_dev.yaml",
    ".\levoit-vital200s\levoit-vital200s_dev.yaml",
    ".\levoit-sprout\levoit-sprout_dev.yaml"
)

$failed = @()

foreach ($config in $configs) {
    Write-Host "`n==> Compiling $config" -ForegroundColor Cyan
    esphome compile $config
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $config" -ForegroundColor Red
        $failed += $config
    }
}

Write-Host ""
if ($failed.Count -eq 0) {
    Write-Host "All builds succeeded." -ForegroundColor Green
} else {
    Write-Host "Failed builds:" -ForegroundColor Red
    $failed | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}
