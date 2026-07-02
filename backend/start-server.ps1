# Traffic Management Backend — Startup Script (MSVC / Visual Studio 2022)
# Run from PowerShell inside the backend/ folder:  .\start-server.ps1

Write-Host ""
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "  Traffic Management Backend — Startup" -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host ""

$binaryPath = ".\build\Release\TrafficManagementBackend.exe"

# ── Build if binary is missing ────────────────────────────────────────────────
if (-not (Test-Path $binaryPath)) {
    Write-Host "Binary not found. Building now..." -ForegroundColor Yellow
    Write-Host ""

    if (-not (Test-Path ".\build")) {
        New-Item -ItemType Directory -Path ".\build" | Out-Null
    }

    Push-Location ".\build"

    $toolchain = if ($env:VCPKG_ROOT) {
        "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
    } else {
        "$HOME\vcpkg\scripts\buildsystems\vcpkg.cmake"
    }

    cmake .. -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"
    if ($LASTEXITCODE -ne 0) { Write-Host "CMake configure failed!" -ForegroundColor Red; Pop-Location; exit 1 }

    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; Pop-Location; exit 1 }

    Pop-Location
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host ""
}

# ── Database password ─────────────────────────────────────────────────────────
$password = Read-Host "Enter PostgreSQL password"
if ([string]::IsNullOrWhiteSpace($password)) {
    Write-Host "A PostgreSQL password is required for native backend runs." -ForegroundColor Red
    exit 1
}

$env:DATABASE_URL = "host=localhost port=5432 dbname=traffic_db user=postgres password=$password"
Write-Host "DATABASE_URL set" -ForegroundColor Green

# ── Start ─────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "Starting server on http://localhost:8080" -ForegroundColor Green
Write-Host "Press Ctrl+C to stop"
Write-Host ""

& $binaryPath
