#include "ExternalTools.h"
#include "SettingsManager.h"
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QWidget>

ExternalTools::ExternalTools() {
}

void ExternalTools::startResizeExternal(const QStringList& files) {
    // Pfad zum Resize-Programm aus den Einstellungen holen
    QString resizeProgram = SettingsManager::getExternalProgramPath("resizeProgram");
    
    if (resizeProgram.isEmpty()) {
        QMessageBox::warning(nullptr, "Fehler", "Pfad zu Resize-Programm nicht gefunden!");
        return;
    }
    
    // Starte das Programm für jede ausgewählte Datei
    for (const QString &file : files) {
        QProcess::startDetached(resizeProgram, QStringList() << file);
    }
}

void ExternalTools::openWithDefaultProgram(const QString& filePath) {
    // Öffne die Datei mit dem Standardprogramm des Betriebssystems
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}
