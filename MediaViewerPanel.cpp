#include "MediaViewerPanel.h"
#include <QMimeDatabase>
#include <QFileInfo>
#include <QStyle>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>

MediaViewerPanel::MediaViewerPanel(QWidget* parent) : QWidget(parent), zoomFactor(1.0), videoLoopEnabled(false) {
    // Lade Lautstärke und Loop-Status aus der Konfigurationsdatei
    double volumeValue = 0.7; // Standardwert
    
    QFile configFile("build/program-settings.json");
    if(configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject configObj = configDoc.object();
        
        if(configObj.contains("volume"))
            volumeValue = configObj["volume"].toDouble();
            
        if(configObj.contains("videoLoop"))
            videoLoopEnabled = configObj["videoLoop"].toBool();
            
        configFile.close();
    }
    
    setupUI(volumeValue);
}

MediaViewerPanel::~MediaViewerPanel() {
    // Aufräumen, falls nötig
}

void MediaViewerPanel::setupUI(double volumeValue) {
    // Layout erstellen
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar erstellen
    mediaToolBar = new QToolBar(this);
    mediaToolBar->setMovable(false);
    mediaToolBar->setFloatable(false);
    mediaToolBar->setIconSize(QSize(24, 24)); // Größere Icons für bessere Sichtbarkeit
    mediaToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mediaToolBar->setStyleSheet("QToolBar { spacing: 5px; }"); // Mehr Abstand zwischen Buttons
    
    // Aktionen hinzufügen
    QStyle* style = QApplication::style();
    
    // Play/Pause-Button
    playPauseAction = new QAction(style->standardIcon(QStyle::SP_MediaPlay), "Abspielen/Pause", this);
    connect(playPauseAction, &QAction::triggered, this, &MediaViewerPanel::onPlayPause);
    
    // Stop-Button
    stopAction = new QAction(style->standardIcon(QStyle::SP_MediaStop), "Stopp", this);
    connect(stopAction, &QAction::triggered, this, &MediaViewerPanel::onStop);
    
    // Vorheriger Frame-Button
    prevAction = new QAction(style->standardIcon(QStyle::SP_MediaSkipBackward), "Vorheriger Frame", this);
    connect(prevAction, &QAction::triggered, this, &MediaViewerPanel::onPrev);
    
    // Nächster Frame-Button
    nextAction = new QAction(style->standardIcon(QStyle::SP_MediaSkipForward), "Nächster Frame", this);
    connect(nextAction, &QAction::triggered, this, &MediaViewerPanel::onNext);
    
    // Screenshot-Button
    screenshotAction = new QAction(style->standardIcon(QStyle::SP_DesktopIcon), "Screenshot", this);
    connect(screenshotAction, &QAction::triggered, this, &MediaViewerPanel::onScreenshot);
    
    // Loop-Button (checkbar) mit deutlicherem Icon
    QIcon loopIcon;
    if (QFile::exists("icons/loop.png")) {
        loopIcon = QIcon("icons/loop.png");
    } else {
        // Fallback auf ein Standard-Icon, das besser erkennbar ist
        loopIcon = style->standardIcon(QStyle::SP_BrowserReload);
    }
    
    loopAction = new QAction(loopIcon, "LOOP", this);
    loopAction->setCheckable(true);
    loopAction->setChecked(videoLoopEnabled); // Status aus Konfiguration laden
    loopAction->setToolTip("Video in Endlosschleife abspielen"); // Tooltip hinzufügen
    connect(loopAction, &QAction::toggled, this, &MediaViewerPanel::onLoopToggled);
    
    // Verbinde MediaPlayer mit MediaStatusChanged für Loop-Funktion
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MediaViewerPanel::onMediaStatusChanged);
    
    // Lautstärke-Widget
    volumeWidget = new QWidget(this);
    QHBoxLayout* volumeLayout = new QHBoxLayout(volumeWidget);
    volumeLayout->setContentsMargins(5, 0, 5, 0);
    
    QLabel* volumeIcon = new QLabel(this);
    volumeIcon->setPixmap(style->standardIcon(QStyle::SP_MediaVolume).pixmap(16, 16));
    
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(volumeValue * 100); // Wert aus der Konfigurationsdatei setzen
    volumeSlider->setFixedWidth(80);
    connect(volumeSlider, &QSlider::valueChanged, this, &MediaViewerPanel::onVolumeChanged);
    
    volumeLayout->addWidget(volumeIcon);
    volumeLayout->addWidget(volumeSlider);
    
    // Aktionen zur Toolbar hinzufügen
    mediaToolBar->addAction(playPauseAction);
    mediaToolBar->addAction(stopAction);
    mediaToolBar->addSeparator(); // Trenner für bessere Gruppierung
    mediaToolBar->addAction(prevAction);
    mediaToolBar->addAction(nextAction);
    mediaToolBar->addSeparator(); // Trenner für bessere Gruppierung
    
    // Eigener Button für Loop mit besserer Sichtbarkeit
    QToolButton* loopButton = new QToolButton();
    loopButton->setDefaultAction(loopAction);
    loopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    loopButton->setMinimumWidth(80); // Breiter machen für bessere Sichtbarkeit
    loopButton->setStyleSheet("QToolButton { padding: 5px; border: 1px solid #aaa; border-radius: 3px; } "
                            "QToolButton:checked { background-color: #88c0d0; color: white; font-weight: bold; }");
    
    mediaToolBar->addWidget(loopButton);
    
    mediaToolBar->addSeparator(); // Trenner für bessere Gruppierung
    mediaToolBar->addAction(screenshotAction);
    mediaToolBar->addWidget(volumeWidget);
    
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
    
    // Setze die Lautstärke aus dem Parameter
    audioOutput->setVolume(volumeValue);
    
    // Verbinde MediaPlayer mit MediaStatusChanged für Loop-Funktion
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MediaViewerPanel::onMediaStatusChanged);
    
    // Widgets zum StackedWidget hinzufügen
    stackedWidget->addWidget(imageLabel);      // Index 0: Bild
    stackedWidget->addWidget(videoWidget);     // Index 1: Video
    
    // Container für Playback-Slider
    QWidget* sliderContainer = new QWidget(this);
    QHBoxLayout* sliderLayout = new QHBoxLayout(sliderContainer);
    sliderLayout->setContentsMargins(10, 5, 10, 5); // Vertikaler Abstand
    
    // Playback-Position-Slider - größer und besser sichtbar
    positionSlider = new QSlider(Qt::Horizontal, sliderContainer);
    positionSlider->setRange(0, 1000); // Wir verwenden 0-1000 für Genauigkeit
    positionSlider->setMinimumHeight(20); // Höher machen
    
    // Style für den Slider
    QString sliderStyle = "QSlider::groove:horizontal {height: 8px; background: #ccc; margin: 2px 0;} "
                         "QSlider::handle:horizontal {background: #007bff; border: 1px solid #5c5c5c; width: 18px; margin: -5px 0; border-radius: 9px;}";
    positionSlider->setStyleSheet(sliderStyle);
    
    // Bei Klick im Slider sofort zur Position springen
    connect(positionSlider, &QSlider::sliderPressed, this, [this]() {
        // Wenn der Slider gedrückt wird, sofort zur Position springen
        qint64 duration = mediaPlayer->duration();
        if (duration > 0) {
            int value = positionSlider->value();
            mediaPlayer->setPosition(value * duration / 1000);
        }
    });
    
    // Bei Bewegung des Sliders zur Position springen
    connect(positionSlider, &QSlider::sliderMoved, this, [this](int position) {
        // Konvertiere Slider-Position (0-1000) zu Millisekunden
        qint64 duration = mediaPlayer->duration();
        if (duration > 0) {
            mediaPlayer->setPosition(position * duration / 1000);
        }
    });
    
    // Verbinde MediaPlayer-Positionsupdate mit Slider
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        qint64 duration = mediaPlayer->duration();
        if (duration > 0 && !positionSlider->isSliderDown()) {
            // Konvertiere Position zu Slider-Wert (0-1000)
            positionSlider->setValue(position * 1000 / duration);
        }
    });
    
    // Verbinde Daueränderung mit Slider-Reset
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        positionSlider->setValue(0);
    });
    
    sliderLayout->addWidget(positionSlider);
    
    // Layout zusammenbauen
    mainLayout->addWidget(mediaToolBar);
    mainLayout->addWidget(stackedWidget);
    mainLayout->addWidget(sliderContainer);
    
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

void MediaViewerPanel::onVolumeChanged(int value) {
    double volume = value / 100.0;
    audioOutput->setVolume(volume);
    
    // Speichere die Lautstärke in der Konfigurationsdatei
    QFile configFile("build/program-settings.json");
    if(configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject configObj = configDoc.object();
        
        // Aktualisiere den Lautstärkewert
        configObj["volume"] = volume;
        
        // Erstelle ein neues QJsonDocument mit dem aktualisierten Objekt
        QJsonDocument updatedDoc(configObj);
        
        configFile.close();
        
        // Schreibe die aktualisierte Konfiguration zurück
        if(configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            // Verwende QJsonDocument::Indented für besser lesbares JSON
            configFile.write(updatedDoc.toJson(QJsonDocument::Indented));
            configFile.close();
            
            qDebug() << "Lautstärke gespeichert:" << volume;
        } else {
            qDebug() << "Fehler beim Öffnen der Konfigurationsdatei zum Schreiben";
        }
    } else {
        qDebug() << "Fehler beim Öffnen der Konfigurationsdatei zum Lesen";
    }
}

void MediaViewerPanel::onPrev() {
    // Ein Frame zurückspringen (ca. 33ms für 30fps)
    qint64 currentPosition = mediaPlayer->position();
    qint64 newPosition = qMax(currentPosition - 33, (qint64)0);
    mediaPlayer->setPosition(newPosition);
}

void MediaViewerPanel::onNext() {
    // Ein Frame vorspringen (ca. 33ms für 30fps)
    qint64 currentPosition = mediaPlayer->position();
    qint64 duration = mediaPlayer->duration();
    qint64 newPosition = qMin(currentPosition + 33, duration);
    mediaPlayer->setPosition(newPosition);
}

void MediaViewerPanel::onLoopToggled(bool checked) {
    // Speichere den Loop-Status
    videoLoopEnabled = checked;
    
    // Speichere den Status in der Konfigurationsdatei
    QFile configFile("build/program-settings.json");
    if(configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject configObj = configDoc.object();
        
        // Aktualisiere den Loop-Status
        configObj["videoLoop"] = videoLoopEnabled;
        
        // Erstelle ein neues QJsonDocument mit dem aktualisierten Objekt
        QJsonDocument updatedDoc(configObj);
        
        configFile.close();
        
        // Schreibe die aktualisierte Konfiguration zurück
        if(configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            // Verwende QJsonDocument::Indented für besser lesbares JSON
            configFile.write(updatedDoc.toJson(QJsonDocument::Indented));
            configFile.close();
            
            qDebug() << "Loop-Status gespeichert:" << (videoLoopEnabled ? "aktiviert" : "deaktiviert");
        } else {
            qDebug() << "Fehler beim Öffnen der Konfigurationsdatei zum Schreiben";
        }
    } else {
        qDebug() << "Fehler beim Öffnen der Konfigurationsdatei zum Lesen";
    }
}

void MediaViewerPanel::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    // Wenn das Video zu Ende ist und Loop aktiviert ist, starte es neu
    if (status == QMediaPlayer::EndOfMedia && videoLoopEnabled) {
        mediaPlayer->setPosition(0);
        mediaPlayer->play();
    }
}

void MediaViewerPanel::onPlayPause() {
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer->pause();
        playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    } else {
        mediaPlayer->play();
        playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
    }
}

void MediaViewerPanel::onStop() {
    mediaPlayer->stop();
    playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
}

void MediaViewerPanel::onMediaStateChanged(QMediaPlayer::PlaybackState state) {
    // Aktualisiere das Play/Pause-Icon basierend auf dem Wiedergabestatus
    if (state == QMediaPlayer::PlayingState) {
        playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void MediaViewerPanel::onCopyImage() {
    // Kopiere das aktuelle Bild oder Video-Frame in die Zwischenablage
    QPixmap screenshot;
    
    if (stackedWidget->currentIndex() == 0) {
        // Bild-Modus
        if (!originalPixmap.isNull()) {
            screenshot = originalPixmap;
        }
    } else {
        // Video-Modus
        screenshot = videoWidget->grab();
    }
    
    if (!screenshot.isNull()) {
        // Verwende QGuiApplication statt QApplication für Zwischenablage
        QGuiApplication::clipboard()->setPixmap(screenshot);
        QMessageBox::information(this, "Bild kopiert", "Das Bild wurde in die Zwischenablage kopiert.");
    }
}

void MediaViewerPanel::onScreenshot() {
    // Lade Screenshot-Pfad aus der Konfigurationsdatei
    QString screenshotsPath = "screenshots";
    
    // Versuche, den Pfad aus der Konfigurationsdatei zu laden
    QFile configFile("build/program-settings.json");
    if(configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject configObj = configDoc.object();
        
        if(configObj.contains("screenshotsPath"))
            screenshotsPath = configObj["screenshotsPath"].toString();
            
        configFile.close();
    }
    
    // Stelle sicher, dass der Pfad existiert
    QDir dir(screenshotsPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Erstelle einen eindeutigen Dateinamen mit Zeitstempel
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename;
    
    if (!currentFilePath.isEmpty()) {
        QFileInfo fileInfo(currentFilePath);
        filename = fileInfo.baseName() + "_" + timestamp + ".png";
    } else {
        filename = "screenshot_" + timestamp + ".png";
    }
    
    QString fullPath = screenshotsPath + "/" + filename;
    
    // Erstelle Screenshot vom aktuellen Frame
    if (stackedWidget->currentIndex() == 0) {
        // Bild-Modus
        if (!originalPixmap.isNull()) {
            originalPixmap.save(fullPath, "PNG");
            QMessageBox::information(this, "Screenshot", "Bild gespeichert als:\n" + fullPath);
        }
    } else {
        // Video-Modus
        QPixmap screenshot = videoWidget->grab();
        if (!screenshot.isNull()) {
            screenshot.save(fullPath, "PNG");
            QMessageBox::information(this, "Screenshot", "Screenshot gespeichert als:\n" + fullPath);
        } else {
            QMessageBox::warning(this, "Screenshot fehlgeschlagen", 
                "Das Video konnte nicht als Screenshot gespeichert werden.\n"
                "Dies tritt häufig auf, wenn die Hardwarebeschleunigung aktiv ist oder das Video nicht sichtbar ist.\n"
                "Tipp: Pausiere das Video und stelle sicher, dass das Fenster nicht minimiert oder verdeckt ist.\n"
                "Screenshots von Videos sind nicht auf allen Systemen zuverlässig möglich.");
        }
    }
}
