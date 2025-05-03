#include "ScrollConfig.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

int ScrollConfig::navigationAreaPercent() {
    static int percent = -1;
    if (percent < 0) {
        QString path = QCoreApplication::applicationDirPath() + "/program-settings.json";
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                percent = doc.object().value(QStringLiteral("scrollNavigationPercent")).toInt(25);
            }
            f.close();
        } else {
            percent = 25; // default
        }
    }
    return percent;
}
