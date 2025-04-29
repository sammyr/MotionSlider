# PowerShell-Skript: Build starten und Fehler extrahieren
$buildOutput = & mingw32-make -C build 2>&1
Write-Host $buildOutput
if ($buildOutput -match "error: (.+)") {
    Write-Host "`n>>> Nächster Fehler gefunden:"
    $matches[1] | Write-Host
} else {
    Write-Host "`nBuild erfolgreich!"
}
