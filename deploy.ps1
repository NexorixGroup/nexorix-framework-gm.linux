# ============================================================
#  deploy.ps1 — Compila o gamemode e copia binários para gm-nexorix
#  Uso: .\deploy.ps1
#       .\deploy.ps1 -FullBuild   (recompila servidor C++ também)
# ============================================================

param(
    [switch]$FullBuild
)

$ErrorActionPreference = "Stop"
$ServerDir  = "gm-nexorix"
$BuildDir   = "build"
$PawnCC     = "$ServerDir\sdk\pawncc.exe"
$ScriptSrc  = "$ServerDir\scripts\gm.pwn"
$ScriptOut  = "$ServerDir\scripts"

Write-Host ""
Write-Host "══════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Nexorix Deploy Script" -ForegroundColor Cyan
Write-Host "══════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# ── 1. Compilar gamemode Pawn ─────────────────────────────────
Write-Host "[1/3] Compilando gamemode..." -ForegroundColor Yellow

if (-not (Test-Path $PawnCC)) {
    Write-Error "pawncc.exe não encontrado em $PawnCC"
    exit 1
}

& $PawnCC $ScriptSrc "-D$ScriptOut" "-i$ServerDir\sdk\include" "-;+" "-(+" "-d3"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERRO] Falha na compilação do gamemode." -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Gamemode compilado." -ForegroundColor Green

# ── 2. Build C++ (opcional) ───────────────────────────────────
if ($FullBuild) {
    Write-Host "[2/3] Compilando servidor C++..." -ForegroundColor Yellow
    cmake --build $BuildDir --config RelWithDebInfo --target Server
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERRO] Falha no build C++." -ForegroundColor Red
        exit 1
    }

    Write-Host "[2/3] Copiando binários..." -ForegroundColor Yellow
    Copy-Item "$BuildDir\Output\RelWithDebInfo\Server\nexorix-server.exe" "$ServerDir\nexorix-server.exe" -Force
    Copy-Item "$BuildDir\Output\RelWithDebInfo\Server\nexorix-server.pdb" "$ServerDir\nexorix-server.pdb" -Force
    Copy-Item "$BuildDir\Output\RelWithDebInfo\Server\mods\*"             "$ServerDir\mods\" -Force
    Write-Host "[OK] Binários copiados." -ForegroundColor Green
} else {
    Write-Host "[2/3] Pulando build C++ (use -FullBuild para incluir)." -ForegroundColor DarkGray
}

# ── 3. Criar pasta de contas se não existir ───────────────────
Write-Host "[3/3] Verificando estrutura de pastas..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "$ServerDir\data\contas" -Force | Out-Null
Write-Host "[OK] Estrutura OK." -ForegroundColor Green

Write-Host ""
Write-Host "══════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Deploy concluído!" -ForegroundColor Green
Write-Host "  Inicie o servidor: $ServerDir\nexorix-server.exe" -ForegroundColor Cyan
Write-Host "══════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
