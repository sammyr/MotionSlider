#pragma once

#include <QWidget>
#include <QLabel>
#include <QMediaPlayer>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QSlider>
#include <QAudioOutput> // Für Audio-Unterstützung
#include <QPixmap>
#include <QToolButton>
#include <QClipboard>
#include <QGuiApplication>
#include <QScrollArea> // Für Bild-Panning
#include "VideoGraphicsView.h"

class MediaViewerPanel : public QWidget {
    Q_OBJECT
public:
    explicit MediaViewerPanel(QWidget* parent = nullptr);
    ~MediaViewerPanel();
    void clearMedia();
    
    // Dateityp ermitteln und entsprechenden Viewer anzeigen
    void loadFile(const QString& filePath);
    
    // Bild laden und anzeigen
    void loadImage(const QString& imagePath);
    
    // Video laden und anzeigen
    void loadVideo(const QString& videoPath);
    
    // UI-Komponenten zurückgeben
    QStackedWidget* getStackedWidget() const { return stackedWidget; }
    QLabel* getImageLabel() const { return imageLabel; }
    VideoGraphicsView* getVideoView() const { return videoView; }
    QMediaPlayer* getMediaPlayer() const { return mediaPlayer; }
    
    // Zoom für Bilder
    void zoomImage(double factor);
    
private:
    // Overlay für die Dateigröße
    QLabel* sizeOverlay = nullptr;

    // UI-Komponenten
    QStackedWidget* stackedWidget = nullptr;
    QLabel* imageLabel = nullptr;
    QGraphicsScene* videoScene = nullptr;
    QGraphicsVideoItem* videoItem = nullptr;
    VideoGraphicsView* videoView = nullptr;
    QMediaPlayer* mediaPlayer = nullptr;
    QAudioOutput* audioOutput = nullptr; // Für die Tonwiedergabe
    bool mutedEnabled = false; // Status für Stummschaltung
    QVBoxLayout* mainLayout = nullptr;
    QToolBar* mediaToolBar = nullptr;
    QAction* playPauseAction = nullptr;
    QAction* stopAction = nullptr;
    QAction* prevAction = nullptr;
    QAction* nextAction = nullptr;
    QAction* screenshotAction = nullptr;
    QAction* copyImageAction = nullptr;
    QAction* loopAction = nullptr; // Loop-Button für Endlosschleife
    QSlider* volumeSlider = nullptr;
    QSlider* positionSlider = nullptr; // Für Video-Position
    QWidget* volumeWidget = nullptr; // Für Slider-Einbettung
    QWidget* sliderContainer = nullptr; // Container für Timeline
    QScrollArea* imageScrollArea = nullptr; // Für Bild-Panning

    // Bilddaten
    QPixmap originalPixmap;
    double zoomFactor = 1.0;
    QString currentFilePath;
    bool videoLoopEnabled = false; // Status für Loop-Funktion
    bool isPanning = false;
    QPoint lastPanPoint;
    QString externalVideoPlayerPath; // Pfad zum externen Video-Player aus program-settings.json
    QString externalImageEditorPath; // Pfad zum externen Bild-Editor aus program-settings.json

    // UI initialisieren
    void setupUI(double volumeValue = 0.7);
    void setupVideoToolbar();
    void setupImageToolbar();
    void clearToolbar();

    // Aktualisiert Position und Größe des Overlays
    void updateSizeOverlay();

signals:
    void customContextMenuRequested(const QPoint &pos);

private slots:
    void onLoopToggled(bool checked);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayPause();
    void onStop();
    void onPrev();
    void onNext();
    void onScreenshot();
    void onCopyImage();
    void onVolumeChanged(int value);
    void onMediaStateChanged(QMediaPlayer::PlaybackState state);
    void showContextMenu(const QPoint &pos);  // Rechtsklick-Menü

protected:
    // Fenstergröße oder Splitter bewegt
    void resizeEvent(QResizeEvent* event) override;
    // Zoomen des Bildes per Mausrad im rechten Panel
    void wheelEvent(QWheelEvent *event) override;
    // Panning per mittlere Maustaste
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // Panning-Event-Filter für ScrollArea-Viewport
    bool eventFilter(QObject *watched, QEvent *event) override;

    // Passt Bild an Fenstergröße an und erhält das Seitenverhältnis
    void fitImageToWindow();
    // Passt Video an Fenstergröße an
    void fitVideoToWindow();

};
