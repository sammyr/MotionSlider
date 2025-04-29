#include "MediaViewerPanel.h"
#include <QMimeDatabase>
#include <QFileInfo>
#include <QStyle>
#include <QApplication>

MediaViewerPanel::MediaViewerPanel(QWidget* parent) : QWidget(parent), zoomFactor(1.0) {
    setupUI();
}

MediaViewerPanel::~MediaViewerPanel() {
    // Aufräumen, falls nötig
}

void MediaViewerPanel::setupUI() {
    // Layout erstellen
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar erstellen
    mediaToolBar = new QToolBar(this);
    mediaToolBar->setMovable(false);
    mediaToolBar->setFloatable(false);
    mediaToolBar->setIconSize(QSize(20, 20));
    mediaToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // Aktionen hinzufügen
    QStyle* style = QApplication::style();
    QAction* infoAction = new QAction(style->standardIcon(QStyle::SP_MessageBoxInformation), "Info", this);
    QAction* saveAction = new QAction(style->standardIcon(QStyle::SP_DialogSaveButton), "Speichern", this);
    mediaToolBar->addAction(infoAction);
    mediaToolBar->addAction(saveAction);
    
    // StackedWidget für Bild/Video
    stackedWidget = new QStackedWidget(this);
    
    // Bild-Anzeige
    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    
    // Video-Anzeige mit Audio
    videoWidget = new QVideoWidget;
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this); // Neuer Audio-Output
    mediaPlayer->setAudioOutput(audioOutput); // Audio-Output setzen
    mediaPlayer->setVideoOutput(videoWidget);
    
    // Standard-Lautstärke auf 70%
    audioOutput->setVolume(0.7);
    
    // Widgets zum StackedWidget hinzufügen
    stackedWidget->addWidget(imageLabel);      // Index 0: Bild
    stackedWidget->addWidget(videoWidget);     // Index 1: Video
    
    // Layout zusammenbauen
    mainLayout->addWidget(mediaToolBar);
    mainLayout->addWidget(stackedWidget);
    
    // Anfangszustand: beide ausblenden
    imageLabel->hide();
    videoWidget->hide();
}

void MediaViewerPanel::loadFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) return;
    
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filePath);
    
    if (type.name().startsWith("image/")) {
        loadImage(filePath);
    } else if (type.name().startsWith("video/") || 
              filePath.endsWith(".mp4", Qt::CaseInsensitive) ||
              filePath.endsWith(".avi", Qt::CaseInsensitive) ||
              filePath.endsWith(".mkv", Qt::CaseInsensitive) ||
              filePath.endsWith(".mov", Qt::CaseInsensitive) ||
              filePath.endsWith(".flv", Qt::CaseInsensitive) ||
              filePath.endsWith(".wmv", Qt::CaseInsensitive)) {
        loadVideo(filePath);
    }
}

void MediaViewerPanel::loadImage(const QString& imagePath) {
    // Video stoppen falls läuft
    mediaPlayer->stop();
    videoWidget->hide();
    
    // Bild laden
    originalPixmap.load(imagePath);
    if (originalPixmap.isNull()) return;
    
    // Bild anzeigen
    imageLabel->setPixmap(originalPixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->show();
    stackedWidget->setCurrentIndex(0);
    
    // Zoom zurücksetzen
    zoomFactor = 1.0;
}

void MediaViewerPanel::loadVideo(const QString& videoPath) {
    // Bild ausblenden
    imageLabel->hide();
    
    // Video laden und abspielen
    mediaPlayer->setSource(QUrl::fromLocalFile(videoPath));
    videoWidget->show();
    stackedWidget->setCurrentIndex(1);
    mediaPlayer->play();
}

void MediaViewerPanel::zoomImage(double factor) {
    zoomFactor = factor;
    if (originalPixmap.isNull()) return;
    
    // Skalierte Größe berechnen
    QSize scaledSize = originalPixmap.size() * zoomFactor;
    
    // Bild neu skalieren und anzeigen
    imageLabel->setPixmap(originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
