#pragma once

#include <QWidget>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
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
    QStackedWidget* stackedWidget = nullptr;
    QLabel* imageLabel = nullptr;
    QVideoWidget* videoWidget = nullptr;
    QMediaPlayer* mediaPlayer = nullptr;
    QAudioOutput* audioOutput = nullptr; // Für die Tonwiedergabe
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
    QScrollArea* imageScrollArea = nullptr; // Für Bild-Panning

    // Bilddaten
    QPixmap originalPixmap;
    double zoomFactor = 1.0;
    QString currentFilePath;
    bool videoLoopEnabled = false; // Status für Loop-Funktion
    bool isPanning = false;
    QPoint lastPanPoint;

    // UI initialisieren
    void setupUI(double volumeValue = 0.7);
    void setupVideoToolbar();
    void setupImageToolbar();
    void clearToolbar();

private slots:
    void onPlayPause();
    void onStop();
    void onPrev();
    void onNext();
    void onScreenshot();
    void onCopyImage();
    void onVolumeChanged(int value);
    void onLoopToggled(bool checked);
    void onMediaStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

protected:
    // Zoomen des Bildes per Mausrad im rechten Panel
    void wheelEvent(QWheelEvent *event) override;
    // Panning per mittlere Maustaste
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // Panning-Event-Filter für ScrollArea-Viewport
    bool eventFilter(QObject *watched, QEvent *event) override;
};
