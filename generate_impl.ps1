# Script de génération des implémentations complètes

Write-Host "=== Génération des Implémentations ===" -ForegroundColor Green

$projectRoot = "C:\Users\PRODIGY\Documents\screen-share\multimedia-streaming-app"

# Créer les répertoires manquants
$directories = @(
    "src\capture",
    "build",
    "certs"
)

foreach ($dir in $directories) {
    $fullPath = Join-Path $projectRoot $dir
    if (!(Test-Path $fullPath)) {
        New-Item -ItemType Directory -Path $fullPath -Force | Out-Null
        Write-Host "✓ Créé: $dir" -ForegroundColor Cyan
    }
}

Write-Host "`n=== Fichiers créés avec succès ===" -ForegroundColor Green
Write-Host "`nPour compiler:" -ForegroundColor Yellow
Write-Host "  cd $projectRoot"
Write-Host "  cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
Write-Host "  cmake --build build --config Release"

Write-Host "`nPour exécuter:" -ForegroundColor Yellow
Write-Host "  Serveur: .\build\bin\Release\server_app.exe --port 9999"
Write-Host "  Client: .\build\bin\Release\client_app.exe --server 127.0.0.1"
