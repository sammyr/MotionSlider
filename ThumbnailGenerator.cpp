#include "ThumbnailGenerator.h"
#include "ThumbnailDelegate.h"
#include "SettingsManager.h"
#include <QJsonObject>
#include <QDir>
#include <QMimeDatabase>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QCoreApplication>
#include <QProcess>
#include <QTemporaryFile>
#include <QDateTime>
#include <QMetaType>
#include <QApplication>

//=== ThumbnailWorker Implementierung ===//

ThumbnailWorker::ThumbnailWorker() : m_stopRequested(false) {
    // Thumbnail-Größe aus Einstellungen laden
    QJsonObject settings = SettingsManager::readProgramSettings();
    thumbnailWidth = settings.value("thumbnailWidth").toInt(120); // Fallback auf 120
    thumbnailHeight = settings.value("thumbnailHeight").toInt(110); // Fallback auf 110
    
}

ThumbnailWorker::~ThumbnailWorker() {
}

QStringList ThumbnailWorker::getVideoExtensions() {
    return {"avi", "mp4", "mkv", "mov", "flv", "wmv", "mpg", "mpeg", "m4v", "3gp", "webm"};
}

void ThumbnailWorker::stopProcessing() {
    m_stopRequested = true;
}

void ThumbnailWorker::processThumbnails(const QString& folderPath, QStringList filePaths) {
    // qDebug entfernt "[ThumbnailWorker] processThumbnails gestartet für Ordner:" << folderPath << ", Dateien:" << filePaths.size();
    m_stopRequested = false;
    
    // Sicherstellen, dass das Thumbnails-Verzeichnis existiert
    QDir thumbDir(QCoreApplication::applicationDirPath() + "/thumbnails");
    if (!thumbDir.exists()) {
        thumbDir.mkpath(".");
    }
    
    int total = filePaths.size();
    int current = 0;
    
    for (const QString& filePath : filePaths) {
        // qDebug entfernt "[ThumbnailWorker] Versuche Thumbnail für:" << filePath;

        if (m_stopRequested) {
            break;
        }
        
        // Thumbnail generieren
        bool success = generateThumbnailForFile(filePath);
        // qDebug entfernt "[ThumbnailWorker] Ergebnis generateThumbnailForFile(" << filePath << ") : " << success;
        if (success) {
            emit thumbnailGenerated(filePath);
        }
        
        // Fortschritt aktualisieren
        current++;
        emit progressUpdate(current, total);
        
        // Kurze Pause, um die UI nicht zu blockieren
        QApplication::processEvents();
    }
    
    emit finished();
}

bool ThumbnailWorker::generateThumbnailForFile(const QString& filePath) {
    // qDebug entfernt "[ThumbnailWorker] generateThumbnailForFile aufgerufen für:" << filePath;
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile() || info.suffix().isEmpty()) {
        return false;
    }
    
    QString ext = info.suffix().toLower();
    QString thumbPath = QCoreApplication::applicationDirPath() + "/thumbnails/" + 
                      info.completeBaseName() + "_" + ext + "_thumb.png";
    
    // Überprüfen, ob das Thumbnail bereits existiert
    if (QFile::exists(thumbPath)) {
        return true; // Thumbnail existiert bereits
    }
    
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(info.filePath());
    
    if (type.name().startsWith("image/")) {
        
        QPixmap pix(info.filePath());
        if (!pix.isNull()) {
            QPixmap thumb = pix.scaled(QSize(thumbnailWidth, thumbnailHeight), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            bool ok = thumb.save(thumbPath, "PNG");
            
            return ok;
        } else {
            
        }
    } else if (type.name().startsWith("video/") || getVideoExtensions().contains(ext)) {
        
        return generateVideoThumbnail(info, thumbPath, QSize(thumbnailWidth, thumbnailHeight));
    }
    
    return false;
}

bool ThumbnailWorker::generateVideoThumbnail(const QFileInfo& info, const QString& thumbPath, const QSize& size) {
    // Pfad zum FFmpeg-Binary
    QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
    if (!QFile::exists(ffmpegPath)) {
        ffmpegPath = "ffmpeg.exe"; // Versuche FFmpeg aus dem System-PATH
    }
    
    // Temporären Dateinamen für das extrahierte Frame
    QString tempImagePath = QCoreApplication::applicationDirPath() + "/thumbnails/temp_" + 
                         QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz") + ".jpg";
    
    // FFmpeg-Befehl zusammenstellen
    QStringList arguments;
    arguments << "-y"; // Überschreibe vorhandene Dateien
    arguments << "-ss" << "00:00:02"; // Springe zu 2 Sekunden im Video
    arguments << "-i" << info.filePath(); // Eingabedatei
    arguments << "-vframes" << "1"; // Extrahiere einen Frame
    arguments << "-q:v" << "2"; // Hohe Qualität
    arguments << tempImagePath; // Ausgabepfad
    
    // FFmpeg-Befehl ausführen
    QProcess ffmpegProcess;
    ffmpegProcess.start(ffmpegPath, arguments);
    
    bool success = false;
    if (ffmpegProcess.waitForStarted(3000)) {
        if (ffmpegProcess.waitForFinished(5000)) {
            if (QFile::exists(tempImagePath)) {
                QPixmap extractedFrame(tempImagePath);
                if (!extractedFrame.isNull()) {
                    // Frame skalieren und für Thumbnail verwenden
                    QPixmap videoThumb = extractedFrame.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    success = videoThumb.save(thumbPath, "PNG");
                    QFile::remove(tempImagePath); // Temporäre Datei löschen
                }
            }
        } else {
            ffmpegProcess.kill();
        }
    }
    
    // Fallback: Bei Fehler generisches Thumbnail erzeugen
    if (!success) {
        QPixmap videoThumb(size);
        videoThumb.fill(Qt::black);
        
        QPainter painter(&videoThumb);
        
        // Farbiger Hintergrund basierend auf Dateiname
        uint nameHash = qHash(info.fileName());
        QColor bgColor;
        bgColor.setHsv(nameHash % 360, 200, 150);
        
        QLinearGradient gradient(0, 0, size.width(), size.height());
        gradient.setColorAt(0, bgColor.lighter(120));
        gradient.setColorAt(1, bgColor.darker(140));
        painter.fillRect(0, 0, 120, 110, gradient);
        
        // Play-Symbol
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.setRenderHint(QPainter::Antialiasing);
        // Play-Symbol dynamisch platzieren
        QPointF points[3] = {
            QPointF(size.width() * 0.33, size.height() * 0.27),
            QPointF(size.width() * 0.33, size.height() * 0.73),
            QPointF(size.width() * 0.75, size.height() * 0.5)
        };
        painter.drawPolygon(points, 3);
        
        // Dateiname
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(7);
        painter.setFont(font);
        
        QString shortName = info.completeBaseName();
        if (shortName.length() > 15) {
            shortName = shortName.left(13) + "...";
        }
        
        QRect textRect(5, size.height() - 30, size.width() - 10, 20);
        painter.fillRect(textRect, QColor(0, 0, 0, 160));
        painter.drawText(textRect, Qt::AlignCenter, shortName);
        
        // Dateigröße
        QString sizeInfo = QString::number(info.size() / (1024*1024.0), 'f', 1) + " MB";
        painter.drawText(QRect(size.width() - 45, 5, 40, 15), Qt::AlignRight, sizeInfo);
        
        if (painter.isActive()) {
            painter.end();
        }
        
        // Speichere das generische Thumbnail
        success = videoThumb.save(thumbPath, "PNG");
    }
    
    return success;
}

//=== ThumbnailGenerator Implementierung ===//

ThumbnailGenerator::ThumbnailGenerator(QObject* parent) : QObject(parent), 
    m_worker(nullptr), m_delegate(nullptr), m_folderView(nullptr), 
    m_isGenerating(false), m_progress(0) {
    
    // Worker im Thread erstellen
    m_worker = new ThumbnailWorker();
    m_worker->moveToThread(&m_workerThread);
    
    // Signal-Slot-Verbindungen herstellen
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ThumbnailWorker::thumbnailGenerated, this, &ThumbnailGenerator::onThumbnailGenerated);
    connect(m_worker, &ThumbnailWorker::progressUpdate, this, &ThumbnailGenerator::onProgressUpdate);
    connect(m_worker, &ThumbnailWorker::finished, this, &ThumbnailGenerator::onGenerationFinished);
    
    // Thread starten
    m_workerThread.start();
}

ThumbnailGenerator::~ThumbnailGenerator() {
    // Worker anhalten und Thread beenden
    if (m_isGenerating) {
        stopThumbnailGeneration();
    }
    
    m_workerThread.quit();
    m_workerThread.wait();
}

void ThumbnailGenerator::startThumbnailGeneration(QListView* folderView, QFileSystemModel* folderModel, ThumbnailDelegate* delegate) {
    // qDebug entfernt "[ThumbnailGenerator] startThumbnailGeneration aufgerufen.";

    if (m_isGenerating || !folderView || !folderModel || !delegate) {
        return;
    }
    
    m_folderView = folderView;
    m_delegate = delegate;
    
    // Status aktualisieren
    m_isGenerating = true;
    m_progress = 0;
    
    // Dem Delegate mitteilen, dass die Generierung begonnen hat
    m_delegate->setGeneratingThumbnails(true);
    
    // Verzeichnispfad abrufen
    QModelIndex rootIdx = folderView->rootIndex();
    QString folderPath = folderModel->filePath(rootIdx);
    
    QDir dir(folderPath);
    if (!dir.exists()) {
        // qDebug entfernt "[ThumbnailGenerator] Verzeichnis existiert nicht:" << folderPath;

        onGenerationFinished();
        return;
    }
    
    // Dateiliste erstellen
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QStringList filePaths;
    // Sicherstellen, dass der Thumbnail-Ordner existiert
    QString thumbDirPath = QCoreApplication::applicationDirPath() + "/thumbnails";
    QDir thumbDir(thumbDirPath);
    if (!thumbDir.exists()) {
        bool ok = thumbDir.mkpath(thumbDirPath);
        // qDebug entfernt "[ThumbnailGenerator] Thumbnail-Ordner erstellt:" << ok << thumbDirPath;
    }
    // Unterstützte Bild- und Video-Formate
    QStringList supportedExtensions = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "mp4", "avi", "mov", "mkv", "wmv", "flv", "webm", "mpg", "mpeg", "m4v", "3gp"};
    // qDebug entfernt "[ThumbnailGenerator] --- ALLE Dateien im Ordner ---";
    for (const QFileInfo &info : dir.entryInfoList()) {
        QString ext = info.suffix().toLower();
        if (!supportedExtensions.contains(ext)) {
            // qDebug entfernt "[ThumbnailGenerator] Ignoriere nicht unterstützten Dateityp:" << info.filePath();
            continue;
        }
        // Thumbnail-Pfad berechnen
        QString thumbPath = QCoreApplication::applicationDirPath() + "/thumbnails/" + 
                         info.completeBaseName() + "_" + ext + "_thumb.png";
        
        // qDebug entfernt "[ThumbnailGenerator] Datei gefunden:" << info.filePath();
        // Nur Dateien ohne vorhandenes Thumbnail hinzufügen
        if (!QFile::exists(thumbPath)) {
            // qDebug entfernt "[ThumbnailGenerator] KEIN Thumbnail vorhanden für:" << info.filePath();
            filePaths.append(info.filePath());
        } else {
            // qDebug entfernt "[ThumbnailGenerator] Thumbnail existiert bereits für:" << info.filePath();
        }
    }
    // qDebug entfernt "[ThumbnailGenerator] --- Dateien ohne Thumbnail (werden generiert): ---" << filePaths;
    
    if (filePaths.isEmpty()) {
        // qDebug entfernt "[ThumbnailGenerator] Keine neuen Thumbnails zu generieren.";
        m_isGenerating = false;
        onGenerationFinished();
        return;
    }
    // qDebug entfernt "[ThumbnailGenerator] Zu generierende Dateien:" << filePaths.size();
    
    // Signal senden und Worker starten
    emit thumbnailGenerationStarted();
    // qDebug entfernt "[ThumbnailGenerator] Worker wird gestartet.";
    QMetaObject::invokeMethod(m_worker, "processThumbnails", Qt::QueuedConnection,
                           Q_ARG(QString, folderPath),
                           Q_ARG(QStringList, filePaths));
}

void ThumbnailGenerator::stopThumbnailGeneration() {
    if (!m_isGenerating) {
        return;
    }
    
    // Worker anhalten
    QMetaObject::invokeMethod(m_worker, "stopProcessing", Qt::QueuedConnection);
}

bool ThumbnailGenerator::isGenerating() const {
    return m_isGenerating;
}

int ThumbnailGenerator::currentProgress() const {
    return m_progress;
}

void ThumbnailGenerator::onThumbnailGenerated(const QString& filePath) {
    // qDebug entfernt "[ThumbnailGenerator] onThumbnailGenerated für:" << filePath;
    // Ein Thumbnail wurde generiert, View aktualisieren
    if (m_folderView) {
        m_folderView->viewport()->update();
    }
}

void ThumbnailGenerator::onProgressUpdate(int current, int total) {
    // qDebug entfernt "[ThumbnailGenerator] onProgressUpdate:" << current << "/" << total;
    if (total > 0) {
        // Prozentsatz berechnen (0-100)
        m_progress = qMin(100, (current * 100) / total);
        emit thumbnailGenerationProgress(m_progress);
    }
}

void ThumbnailGenerator::onGenerationFinished() {
    // qDebug entfernt "[ThumbnailGenerator] onGenerationFinished aufgerufen.";
    // Status aktualisieren
    m_isGenerating = false;
    m_progress = 100;
    

    
    // View aktualisieren
    if (m_folderView) {
        m_folderView->viewport()->update();
    }
    
    emit thumbnailGenerationFinished();
}
