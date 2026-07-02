# Traffic Management System — One-command start
# Usage: .\start-all.ps1

$projectRoot = $PSScriptRoot

function New-RandomSecret([int]$bytes = 32) {
    $buffer = New-Object byte[] $bytes
    [System.Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($buffer)
    return [Convert]::ToBase64String($buffer)
}

function Set-EnvValue([string]$filePath, [string]$key, [string]$value) {
    $escaped = [Regex]::Escape($key)
    $content = Get-Content $filePath -Raw
    $content = [Regex]::Replace($content, "(?m)^$escaped=.*$", "$key=$value")
    Set-Content -Path $filePath -Value $content
}

# Copy .env if it doesn't exist yet
if (-not (Test-Path "$projectRoot\.env")) {
    Copy-Item "$projectRoot\.env.example" "$projectRoot\.env"
    Set-EnvValue "$projectRoot\.env" "POSTGRES_PASSWORD" (New-RandomSecret 18)
    Set-EnvValue "$projectRoot\.env" "DJANGO_SECRET_KEY" (New-RandomSecret 32)
    Set-EnvValue "$projectRoot\.env" "DJANGO_API_KEY" (New-RandomSecret 18)
    Write-Host ".env created from .env.example" -ForegroundColor Yellow
    Write-Host "Local secrets were generated automatically." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Starting Traffic Management System..." -ForegroundColor Cyan
Write-Host "(First run compiles C++ deps - this can take 5-15 min)"
Write-Host ""

Set-Location $projectRoot
docker compose up --build
