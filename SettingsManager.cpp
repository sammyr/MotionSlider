#include "SettingsManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QSplitter>
#include <QWidget>
#include <QRect>
#include <QLineEdit>
#include <QDebug>

SettingsManager::SettingsManager() {
}

void SettingsManager::loadWindowSettings(QWidget* mainWindow, QSplitter* splitter, QLineEdit* pathEdit) {
    QString appDir = QCoreApplication::applicationDirPath();
    QString progPath = appDir + "/program-settings.json";
    // Lade bestehende Programmdaten
    QJsonObject obj;
    QFile progFile(progPath);
    if (progFile.open(QIODevice::ReadOnly)) {
        obj = QJsonDocument::fromJson(progFile.readAll()).object();
        progFile.close();
    }
    // Migriere alte Fenster-Einstellungen
    QString winPath = appDir + "/window-settings.json";
    QFile winFile(winPath);
    if (winFile.open(QIODevice::ReadOnly)) {
        QJsonObject winObj = QJsonDocument::fromJson(winFile.readAll()).object();
        winFile.close();
        // Merge keys
        for (const QString &key : winObj.keys()) {
            obj[key] = winObj[key];
        }
        // Speichere zusammengeführt
        QFile saveFile(progPath);
        if (saveFile.open(QIODevice::WriteOnly)) {
            saveFile.write(QJsonDocument(obj).toJson());
            saveFile.close();
        }
        // alt löschen
        QFile::remove(winPath);
    }
    // Danach obj enthält merged Einstellungen

    // Fensterposition und -größe
    if (obj.contains("window")) {
        QJsonObject win = obj["window"].toObject();
        QRect geom(win["x"].toInt(), win["y"].toInt(), win["width"].toInt(), win["height"].toInt());
        mainWindow->setGeometry(geom);
    }
    
    // Splitter-Position
    if (obj.contains("splitter") && splitter) {
        QJsonArray arr = obj["splitter"].toArray();
        QList<int> sizes;
        for (const QJsonValue &val : arr) {
            sizes.append(val.toInt());
        }
        splitter->setSizes(sizes);
    }
    
    // Letztes Verzeichnis
    if (obj.contains("lastDirectory") && pathEdit) {
        QString lastDir = obj["lastDirectory"].toString();
        pathEdit->setText(lastDir);
    }
}

void SettingsManager::saveWindowSettings(QWidget* mainWindow, QSplitter* splitter, QLineEdit* pathEdit) {
    QString path = QCoreApplication::applicationDirPath() + "/program-settings.json";
    // Bestehende Programmdaten laden
    QJsonObject obj;
    QFile settingsFile(path);
    if (settingsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument docIn = QJsonDocument::fromJson(settingsFile.readAll());
        obj = docIn.object();
        settingsFile.close();
    }
    
    // Fensterposition und -größe
    QRect geom = mainWindow->geometry();
    QJsonObject win;
    win["x"] = geom.x();
    win["y"] = geom.y();
    win["width"] = geom.width();
    win["height"] = geom.height();
    obj["window"] = win;
    
    // Splitter
    if (splitter) {
        QList<int> sizes = splitter->sizes();
        QJsonArray arr;
        for (int s : sizes) {
            arr.append(s);
        }
        obj["splitter"] = arr;
    }
    
    // Letztes Verzeichnis
    if (pathEdit) {
        obj["lastDirectory"] = pathEdit->text();
    }
    
    // Fenster-Daten ins Programm-Settings-Objekt übernehmen
    // (win, splitter, lastDirectory wurden gesetzt)
    QJsonDocument docOut(obj);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(docOut.toJson());
        file.close();
    }
}

QJsonObject SettingsManager::readProgramSettings() {
    // program-settings.json im Programmverzeichnis
    QString settingsPath = QCoreApplication::applicationDirPath() + "/program-settings.json";
    QFile settingsFile(settingsPath);
    QJsonObject settings;
    
    if (settingsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
        settings = doc.object();
        settingsFile.close();
    }
    
    return settings;
}

QString SettingsManager::getExternalProgramPath(const QString& programKey) {
    QJsonObject settings = readProgramSettings();
    QString programPath;
    
    if (settings.contains("externalPrograms")) {
        QJsonObject progs = settings["externalPrograms"].toObject();
        programPath = progs.value(programKey).toString();
    }
    
    return programPath;
}
