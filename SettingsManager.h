#pragma once

#include <QString>
#include <QSplitter>
#include <QWidget>
#include <QJsonObject>
#include <QLineEdit>

class SettingsManager {
public:
    SettingsManager();
    
    // Fenstereinstellungen laden
    static void loadWindowSettings(QWidget* mainWindow, QSplitter* splitter, QLineEdit* pathEdit);
    
    // Fenstereinstellungen speichern
    static void saveWindowSettings(QWidget* mainWindow, QSplitter* splitter, QLineEdit* pathEdit);
    
    // Lese Programmeinstellungen aus program-settings.json
    static QJsonObject readProgramSettings();
    
    // Hilfsklasse, um Pfade zu externen Programmen zu erhalten
    static QString getExternalProgramPath(const QString& programKey);
};
