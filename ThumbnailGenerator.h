#pragma once

#include <QString>
#include <QFileSystemModel>
#include <QListView>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QObject>

class ThumbnailDelegate;

// Worker-Klasse für die asynchrone Generierung von Thumbnails
class ThumbnailWorker : public QObject {
    Q_OBJECT
    
public:
    ThumbnailWorker();
    ~ThumbnailWorker();

public slots:
    // Hauptmethode für die Generierung von Thumbnails (wird im Thread ausgeführt)
    void processThumbnails(const QString& folderPath, QStringList filePaths);
    void stopProcessing();
    
signals:
    // Signale für Fortschritt und Fertigstellung
    void thumbnailGenerated(const QString& filePath);
    void progressUpdate(int current, int total);
    void finished();
    
private:
    // Thumbnail für eine einzelne Datei generieren
    bool generateThumbnailForFile(const QString& filePath);
    // Generiert ein Video-Thumbnail mit FFmpeg oder Fallback
    bool generateVideoThumbnail(const QFileInfo& info, const QString& thumbPath);
    // Liste von bekannten Video-Dateierweiterungen
    QStringList getVideoExtensions();
    
    bool m_stopRequested;
};

class ThumbnailGenerator : public QObject {
    Q_OBJECT
    
public:
    ThumbnailGenerator(QObject* parent = nullptr);
    ~ThumbnailGenerator();
    
    // Starten der Thumbnail-Generierung für einen Ordner
    void startThumbnailGeneration(QListView* folderView, QFileSystemModel* folderModel, ThumbnailDelegate* delegate);
    // Stoppen der aktuellen Thumbnail-Generierung
    void stopThumbnailGeneration();
    // Prüfen, ob gerade Thumbnails generiert werden
    bool isGenerating() const;
    // Fortschritt der Thumbnail-Generierung (0-100%)
    int currentProgress() const;
    
signals:
    // Signale für UI-Updates
    void thumbnailGenerationStarted();
    void thumbnailGenerationProgress(int percent);
    void thumbnailGenerationFinished();
    
private slots:
    // Callback für ein generiertes Thumbnail
    void onThumbnailGenerated(const QString& filePath);
    // Callback für Fortschrittsupdate
    void onProgressUpdate(int current, int total);
    // Callback für abgeschlossene Generierung
    void onGenerationFinished();
    
private:
    QThread m_workerThread;
    ThumbnailWorker* m_worker;
    ThumbnailDelegate* m_delegate;
    QListView* m_folderView;
    bool m_isGenerating;
    int m_progress;
};
