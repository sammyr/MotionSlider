#pragma once

#include <QString>
#include <QStringList>

class ExternalTools {
public:
    ExternalTools();
    
    // Starte Resize-Programm mit ausgewählten Dateien
    static void startResizeExternal(const QStringList& files);
    
    // Öffne ausgewählte Datei mit externem Standardprogramm
    static void openWithDefaultProgram(const QString& filePath);
};
