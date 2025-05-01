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
#include <QWheelEvent>
#include <cmath>
#include <QScrollArea>
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolButton>

#include <QApplication>
#include <QMouseEvent>

// Neuer Slider, der Klicks auf die Groove an Position springt
class ClickableSlider : public QSlider {
public:
    using QSlider::QSlider;
protected:
    void mousePressEvent(QMouseEvent* ev) override {
        if (ev->button() == Qt::LeftButton) {
            int val = QStyle::sliderValueFromPosition(minimum(), maximum(), ev->pos().x(), width());
            setValue(val);
            emit sliderMoved(val);
        }
        QSlider::mousePressEvent(ev);
    }
};

MediaViewerPanel::MediaViewerPanel(QWidget* parent) : QWidget(parent), zoomFactor(1.0), videoLoopEnabled(false), isPanning(false), mutedEnabled(false) {
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
            
        if(configObj.contains("muted"))
            mutedEnabled = configObj["muted"].toBool();
            
        configFile.close();
    }
    
    // Erstelle MediaPlayer und AudioOutput vor UI, damit connects gültig sind
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    
    setupUI(volumeValue);
}

MediaViewerPanel::~MediaViewerPanel() {
    // Aufräumen, falls nötig
}

void MediaViewerPanel::clearMedia() {
    if (mediaPlayer) {
        mediaPlayer->stop();
        mediaPlayer->setSource(QUrl());
    }
    if (videoWidget) videoWidget->hide();
    if (imageLabel) {
        imageLabel->clear();
        imageLabel->hide();
    }
    if (stackedWidget) stackedWidget->setCurrentIndex(0);
    zoomFactor = 1.0;
}

void MediaViewerPanel::setupUI(double volumeValue) {
    // Layout erstellen
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Toolbar erstellen
    mediaToolBar = new QToolBar(this);
    mediaToolBar->setMovable(false);
    mediaToolBar->setFloatable(false);
    mediaToolBar->setIconSize(QSize(24,24));
    mediaToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mediaToolBar->setStyleSheet("QToolBar { spacing:5px; }");

    // Aktionen hinzufügen: Play/Pause, Frame zurück, Frame vor, Loop, Mute, Volume
    // Play/Pause Toggle-Button
    QToolButton* playPauseBtn = new QToolButton(this);
    playPauseBtn->setCheckable(true);
    playPauseBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    connect(playPauseBtn, &QToolButton::toggled, this, [this, playPauseBtn](bool play){
        if(play){ mediaPlayer->play(); playPauseBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause)); }
        else      { mediaPlayer->pause(); playPauseBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay)); }
    });
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [playPauseBtn](QMediaPlayer::PlaybackState st){
        playPauseBtn->setChecked(st==QMediaPlayer::PlayingState);
    });

    // Frame Navigation
    QAction* prevAct = new QAction(QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward),"",this);
    connect(prevAct, &QAction::triggered, this, &MediaViewerPanel::onPrev);
    QAction* nextAct = new QAction(QApplication::style()->standardIcon(QStyle::SP_MediaSkipForward),"",this);
    connect(nextAct, &QAction::triggered, this, &MediaViewerPanel::onNext);

    // Loop Button
    QIcon loopIcon;
    if (QFile::exists("icons/loop.png")) {
        loopIcon = QIcon("icons/loop.png");
    } else {
        // Fallback auf ein Standard-Icon, das besser erkennbar ist
        loopIcon = QApplication::style()->standardIcon(QStyle::SP_BrowserReload);
    }
    
    QAction* loopAction = new QAction(loopIcon, "LOOP", this);
    loopAction->setCheckable(true);
    loopAction->setChecked(videoLoopEnabled); // Status aus Konfiguration laden
    loopAction->setToolTip("Video in Endlosschleife abspielen"); // Tooltip hinzufügen
    connect(loopAction, &QAction::toggled, this, &MediaViewerPanel::onLoopToggled);
    
    QToolButton* loopBtn = new QToolButton(this);
    loopBtn->setDefaultAction(loopAction);
    loopBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    loopBtn->setMinimumWidth(80);

    // Mute Button
    QToolButton* muteButton = new QToolButton(this);
    muteButton->setCheckable(true);
    muteButton->setChecked(mutedEnabled);
    audioOutput->setMuted(mutedEnabled);
    if(mutedEnabled)
        muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    else
        muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
    connect(muteButton, &QToolButton::toggled, this, [this, muteButton](bool muted) {
        audioOutput->setMuted(muted);
        if (muted)
            muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolumeMuted));
        else
            muteButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaVolume));
        // Speichere Mute-Status
        QFile cfg("build/program-settings.json");
        if(cfg.open(QIODevice::ReadWrite)) {
            QJsonObject obj = QJsonDocument::fromJson(cfg.readAll()).object();
            obj["muted"] = muted;
            cfg.resize(0);
            cfg.write(QJsonDocument(obj).toJson());
            cfg.close();
        }
    });

    // Lautstärke-Widget
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(volumeValue * 100); // Wert aus der Konfigurationsdatei setzen
    volumeSlider->setFixedWidth(120);
    connect(volumeSlider, &QSlider::valueChanged, this, &MediaViewerPanel::onVolumeChanged);

    // Add to toolbar
    mediaToolBar->addWidget(playPauseBtn);
    mediaToolBar->addSeparator();
    mediaToolBar->addAction(prevAct);
    mediaToolBar->addAction(nextAct);
    mediaToolBar->addSeparator();
    mediaToolBar->addWidget(loopBtn);
    mediaToolBar->addWidget(muteButton);
    mediaToolBar->addWidget(volumeSlider);
    mediaToolBar->addSeparator(); // Trenner für bessere Gruppierung

    // StackedWidget für Bild/Video
    stackedWidget = new QStackedWidget(this);
    
    // Bild-Anzeige mit ScrollArea für Panning
    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    imageScrollArea = new QScrollArea;
    // Bild in ScrollArea zentrieren
    imageScrollArea->setAlignment(Qt::AlignCenter);
    imageScrollArea->setWidget(imageLabel);
    imageScrollArea->setWidgetResizable(false);
    // Scrollbalken immer ausblenden
    imageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    imageScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Panning: install event filters and enable mouse tracking
    imageScrollArea->installEventFilter(this);
    imageScrollArea->setMouseTracking(true);
    imageScrollArea->viewport()->installEventFilter(this);
    imageScrollArea->viewport()->setMouseTracking(true);
    imageLabel->installEventFilter(this);
    imageLabel->setMouseTracking(true);
    
    // Video-Anzeige mit Audio
    videoWidget = new QVideoWidget;
    mediaPlayer->setVideoOutput(videoWidget);
    audioOutput->setVolume(volumeValue);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MediaViewerPanel::onMediaStatusChanged);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MediaViewerPanel::onMediaStateChanged);
    
    // Widgets zum StackedWidget hinzufügen
    stackedWidget->addWidget(imageScrollArea); // Index 0: Bild mit Panning
    stackedWidget->addWidget(videoWidget);     // Index 1: Video
    
    // Container für Playback-Slider
    QWidget* sliderContainer = new QWidget(this);
    QHBoxLayout* sliderLayout = new QHBoxLayout(sliderContainer);
    sliderLayout->setContentsMargins(10, 5, 10, 5); // Vertikaler Abstand
    
    // Playback-Position-Slider - größer und besser sichtbar
    positionSlider = new ClickableSlider(Qt::Horizontal, sliderContainer);
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
    
    // Slider-Position aktualisieren, wenn das Video läuft
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        qint64 dur = mediaPlayer->duration();
        if (dur > 0) positionSlider->setValue(int(pos * 1000 / dur));
    });
    // Slider zurücksetzen bei neuem Video
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64) {
        positionSlider->setValue(0);
    });

    sliderLayout->addWidget(positionSlider);
    
    // Layout zusammenbauen: Toolbar, Slider, Video/Image
    mainLayout->addWidget(mediaToolBar);
    mainLayout->addWidget(sliderContainer);
    mainLayout->addWidget(stackedWidget);
    
    // Anfangszustand: beide ausblenden
    imageLabel->hide();
    videoWidget->hide();
}

void MediaViewerPanel::loadFile(const QString& filePath) {
    clearMedia();
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
    QSize scaledSize = originalPixmap.size() * zoomFactor;
    QPixmap scaled = originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaled);
    imageLabel->resize(scaled.size()); // Label anpassen für ScrollArea
    imageLabel->show();
    stackedWidget->setCurrentIndex(0);
    
    // Zoom zurücksetzen
    zoomFactor = 1.0;
}

void MediaViewerPanel::loadVideo(const QString& videoPath) {
    // Bild ausblenden
    imageLabel->hide();
    mediaPlayer->setSource(QUrl::fromLocalFile(videoPath));
    videoWidget->show();
    stackedWidget->setCurrentIndex(1);
    positionSlider->setValue(0);
    mediaPlayer->play();
}

void MediaViewerPanel::zoomImage(double factor) {
    if (originalPixmap.isNull()) return;
    // Aktuelle Scroll-Position und Viewport-Maße
    QScrollBar *hBar = imageScrollArea->horizontalScrollBar();
    QScrollBar *vBar = imageScrollArea->verticalScrollBar();
    QWidget *vp = imageScrollArea->viewport();
    QPoint vpCursor = vp->mapFromGlobal(QCursor::pos());
    // Relative Position im gesamten Bild vor Zoom
    double relX = double(hBar->value() + vpCursor.x()) / double(imageLabel->width());
    double relY = double(vBar->value() + vpCursor.y()) / double(imageLabel->height());
    // Zoom-Faktor übernehmen
    zoomFactor = factor;
    // Neue Größe
    QSize scaledSize = originalPixmap.size() * zoomFactor;
    QPixmap scaled = originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaled);
    imageLabel->resize(scaled.size());
    // Scroll auf neue Position entsprechend relX, relY
    int newH = int(relX * imageLabel->width()) - vpCursor.x();
    int newV = int(relY * imageLabel->height()) - vpCursor.y();
    hBar->setValue(qMax(0, qMin(newH, hBar->maximum())));
    vBar->setValue(qMax(0, qMin(newV, vBar->maximum())));
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
    videoLoopEnabled = checked;
}

void MediaViewerPanel::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::MediaStatus::EndOfMedia && videoLoopEnabled) {
        mediaPlayer->setPosition(0);
        mediaPlayer->play();
    }
}

void MediaViewerPanel::onMediaStateChanged(QMediaPlayer::PlaybackState state) {
    if (state == QMediaPlayer::PlaybackState::StoppedState && videoLoopEnabled) {
        mediaPlayer->play();
    }
    // Aktualisiere das Play/Pause-Icon basierend auf dem Wiedergabestatus
    if (state == QMediaPlayer::PlayingState) {
        //playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        //playPauseAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    }
}

void MediaViewerPanel::onPlayPause() {
    // Nicht mehr benötigt
}

void MediaViewerPanel::onStop() {
    // Nicht mehr benötigt
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

// Zoomen des Bildes per Mausrad im rechten Panel
void MediaViewerPanel::wheelEvent(QWheelEvent *event) {
    // Nur bei Bildansicht zoomen
    if (stackedWidget->currentIndex() == 0) {
        int steps = event->angleDelta().y() / 120;
        if (steps != 0) {
            double factor = std::pow(1.1, steps);
            zoomImage(zoomFactor * factor);
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

// Event-Filter für Panning in ScrollArea und Label
bool MediaViewerPanel::eventFilter(QObject *watched, QEvent *event) {
    // Wheel-Event für Zoom (kein Scroll)
    if (event->type() == QEvent::Wheel && stackedWidget->currentIndex() == 0) {
        auto *we = static_cast<QWheelEvent*>(event);
        int steps = we->angleDelta().y() / 120;
        if (steps != 0) {
            double factor = std::pow(1.1, steps);
            zoomImage(zoomFactor * factor);
        }
        return true;
    }
    // Handle panning only on mouse events
    if ((watched == imageScrollArea || watched == imageScrollArea->viewport() || watched == imageLabel)) {
        QEvent::Type type = event->type();
        if (type == QEvent::MouseButtonPress || type == QEvent::MouseMove || type == QEvent::MouseButtonRelease) {
            auto *me = static_cast<QMouseEvent*>(event);
            QPoint vpPos = imageScrollArea->viewport()->mapFromGlobal(me->globalPosition().toPoint());
            if (type == QEvent::MouseButtonPress && (me->button() == Qt::MiddleButton || me->button() == Qt::LeftButton)) {
                isPanning = true;
                lastPanPoint = vpPos;
                imageScrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
                return true;
            }
            if (type == QEvent::MouseMove && isPanning) {
                QPoint delta = vpPos - lastPanPoint;
                lastPanPoint = vpPos;
                imageScrollArea->horizontalScrollBar()->setValue(
                    imageScrollArea->horizontalScrollBar()->value() - delta.x());
                imageScrollArea->verticalScrollBar()->setValue(
                    imageScrollArea->verticalScrollBar()->value() - delta.y());
                return true;
            }
            if (type == QEvent::MouseButtonRelease && (me->button() == Qt::MiddleButton || me->button() == Qt::LeftButton) && isPanning) {
                isPanning = false;
                imageScrollArea->viewport()->setCursor(Qt::ArrowCursor);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Stub-Implementierungen, um fehlende vtable-Einträge zu erfüllen
void MediaViewerPanel::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
}

void MediaViewerPanel::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
}

void MediaViewerPanel::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}
