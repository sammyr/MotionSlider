#pragma once

#include <QWidget>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAudioOutput> // Für Audio-Unterstützung

class MediaViewerPanel : public QWidget {
    Q_OBJECT
public:
    explicit MediaViewerPanel(QWidget* parent = nullptr);
    ~MediaViewerPanel();
    
    // Dateityp ermitteln und entsprechenden Viewer anzeigen
    void loadFile(const QString& filePath);
    
    // Bild laden und anzeigen
    void loadImage(const QString& imagePath);
    
    // Video laden und anzeigen
    void loadVideo(const QString& videoPath);
    
    // UI-Komponenten zurückgeben
    QStackedWidget* getStackedWidget() const { return stackedWidget; }
    QLabel* getImageLabel() const { return imageLabel; }
    QVideoWidget* getVideoWidget() const { return videoWidget; }
    QMediaPlayer* getMediaPlayer() const { return mediaPlayer; }
    
    // Zoom für Bilder
    void zoomImage(double factor);
    
private:
    // UI-Komponenten
    QStackedWidget* stackedWidget;
    QLabel* imageLabel;
    QVideoWidget* videoWidget;
    QMediaPlayer* mediaPlayer;
    QAudioOutput* audioOutput; // Für die Tonwiedergabe
    QVBoxLayout* mainLayout;
    QToolBar* mediaToolBar;
    
    // Bilddaten
    QPixmap originalPixmap;
    double zoomFactor;
    
    // UI initialisieren
    void setupUI();
};
