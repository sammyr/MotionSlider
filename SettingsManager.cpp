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
    QString path = QCoreApplication::applicationDirPath() + "/window-settings.json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        
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
        
        file.close();
    }
}

void SettingsManager::saveWindowSettings(QWidget* mainWindow, QSplitter* splitter, QLineEdit* pathEdit) {
    QString path = QCoreApplication::applicationDirPath() + "/window-settings.json";
    QJsonObject obj;
    
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
    
    QJsonDocument doc(obj);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
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
