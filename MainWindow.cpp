#include "MainWindow.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStatusBar>
#include <QFile>
#include <QRect>
#include <QProcess>
#include <QMessageBox>
#include <QMenuBar>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QSplitter>
#include <QTreeView>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QWheelEvent>
#include <QPushButton>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QDir>
#include <QPainter>
#include <QColor>
#include <QStringList> // Für Navigation History

#include "NameShortenDelegate.h"
#include "ThumbnailDelegate.h"
#include "ThumbnailGenerator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    historyIndex = -1;
    // Menü
    QMenu *fileMenu = menuBar()->addMenu(tr("&Datei"));
    fileMenu->addAction(tr("&Öffnen..."), this, &MainWindow::openFile);

    // QStyle nur einmal deklarieren
    QStyle *style = QApplication::style();

    // Splitter-Layout für verschiebbares Panel
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Linkes Panel: Ordneransicht mit Pfadleiste
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0,0,0,0);

    // Toolbar: Laufwerksauswahl + Pfad + Up-Button
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    // Padding links
    toolbarLayout->addSpacing(5);
    backButton = new QToolButton;
    backButton->setIcon(style->standardIcon(QStyle::SP_ArrowBack));
    backButton->setToolTip("Zurück");
    backButton->setAutoRaise(true);
    backButton->setIconSize(QSize(40,40));
    backButton->setFixedSize(50, 50);
    toolbarLayout->addWidget(backButton);
    forwardButton = new QToolButton;
    forwardButton->setIcon(style->standardIcon(QStyle::SP_ArrowForward));
    forwardButton->setToolTip("Vorwärts");
    forwardButton->setAutoRaise(true);
    forwardButton->setIconSize(QSize(40,40));
    forwardButton->setFixedSize(50, 50);
    toolbarLayout->addWidget(forwardButton);
    // Up-Button: Einen Ordner nach oben navigieren
    upButton = new QToolButton;
    upButton->setIcon(style->standardIcon(QStyle::SP_FileDialogToParent));
    upButton->setToolTip("Nach oben");
    upButton->setAutoRaise(true);
    upButton->setIconSize(QSize(32,32));
    upButton->setFixedSize(36,36);
    toolbarLayout->addWidget(upButton);
    connect(upButton, &QToolButton::clicked, this, &MainWindow::onUpButtonClicked);
    driveComboBox = new QComboBox;
    driveComboBox->setFixedHeight(32);
    driveComboBox->setIconSize(QSize(24,24));
    // Laufwerke eintragen mit Festplattensymbol
    const auto drives = QDir::drives();
    QIcon hddIcon = style->standardIcon(QStyle::SP_DriveHDIcon);
    for (const QFileInfo &drive : drives) {
        driveComboBox->addItem(hddIcon, drive.absoluteFilePath());
    }

    toolbarLayout->addWidget(driveComboBox);
    
    // Pfadfeld
    pathEdit = new QLineEdit;
    pathEdit->setReadOnly(false);
    pathEdit->setMinimumHeight(32);
    toolbarLayout->addWidget(pathEdit, 1);
    
    // Sortierungsauswahlbox hinzufügen
    QComboBox* sortComboBox = new QComboBox();
    sortComboBox->setFixedHeight(32);
    sortComboBox->addItem("Name", "name");
    sortComboBox->addItem("Datum", "date");
    sortComboBox->addItem("Größe", "size");
    sortComboBox->addItem("Typ", "type");
    
    // Aktuellen Sortierwert aus den Einstellungen laden
    QString currentSort = "name"; // Standardwert
    QFile configFile1("build/program-settings.json");
    if(configFile1.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile1.readAll());
        QJsonObject configObj = configDoc.object();
        
        if(configObj.contains("sortOrder"))
            currentSort = configObj["sortOrder"].toString();
            
        configFile1.close();
    }
    
    // Index basierend auf dem aktuellen Sortierwert setzen
    int sortIndex = 0;
    if (currentSort == "date") sortIndex = 1;
    else if (currentSort == "size") sortIndex = 2;
    else if (currentSort == "type") sortIndex = 3;
    // Signals blockieren, damit currentIndexChanged nicht zu früh feuert
    sortComboBox->blockSignals(true);
    sortComboBox->setCurrentIndex(sortIndex);
    sortComboBox->blockSignals(false);
    
    // Verbinde Signal mit Slot zum Speichern der Sortierung
    connect(sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, sortComboBox](int index) {
        QString sortValue = sortComboBox->itemData(index).toString();
        
        // Speichere die Sortierung in der Konfigurationsdatei
        QFile configFile3("build/program-settings.json");
        if(configFile3.open(QIODevice::ReadOnly)) {
            QJsonDocument configDoc = QJsonDocument::fromJson(configFile3.readAll());
            QJsonObject configObj = configDoc.object();
            
            // Aktualisiere den Sortierwert
            configObj["sortOrder"] = sortValue;
            
            // Erstelle ein neues QJsonDocument mit dem aktualisierten Objekt
            QJsonDocument updatedDoc(configObj);
            
            configFile3.close();
            
            // Schreibe die aktualisierte Konfiguration zurück
            if(configFile3.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                configFile3.write(updatedDoc.toJson(QJsonDocument::Indented));
                configFile3.close();
                
                qDebug() << "Sortierung gespeichert:" << sortValue;
            }
        }
        
        // Aktualisiere die Sortierung in der Ansicht
        // Prüfe auf gültige Model- und View-Pointer
        if (!folderModel || !folderView) {
            qDebug() << "Sortierung abgebrochen: Model oder View ist null!";
            return;
        }
        if (sortValue == "name") {
            folderModel->sort(0, Qt::AscendingOrder); // Nach Name sortieren
        } else if (sortValue == "date") {
            folderModel->sort(3, Qt::DescendingOrder); // Nach Datum sortieren (neueste zuerst)
        } else if (sortValue == "size") {
            folderModel->sort(1, Qt::DescendingOrder); // Nach Größe sortieren (größte zuerst)
        } else if (sortValue == "type") {
            folderModel->sort(2, Qt::AscendingOrder); // Nach Typ sortieren
        }
        // View nach Sortierung neu zeichnen
        folderView->update();
        qDebug() << "Sortierung angewendet und View aktualisiert.";
    });
    
    toolbarLayout->addWidget(sortComboBox);
    
    // Padding rechts
    toolbarLayout->addSpacing(5);

    leftLayout->addLayout(toolbarLayout);
    connect(backButton, &QToolButton::clicked, this, &MainWindow::onBackButtonClicked);
    connect(forwardButton, &QToolButton::clicked, this, &MainWindow::onForwardButtonClicked);

    folderView = new QListView;
    folderView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(folderView, &QWidget::customContextMenuRequested, this, &MainWindow::onFolderViewContextMenu);
    leftLayout->addWidget(folderView);
    folderModel = new QFileSystemModel(this);
    folderModel->setRootPath("");
    folderView->setModel(folderModel);
    folderView->viewport()->installEventFilter(this); // Event-Filter für Mausrad: Bei Scroll im rechten Bereich der linken Liste Auswahl ändern
    // Zeige Bild im rechten Panel auch bei Tastatur-Navigation
    connect(folderView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onFolderSelected);

    // Setze benutzerdefinierten Delegate für Thumbnails und konfiguriere View
    m_thumbnailDelegate = new ThumbnailDelegate(folderView);
    folderView->setItemDelegate(m_thumbnailDelegate);
    folderView->setViewMode(QListView::IconMode);
    folderView->setIconSize(QSize(64, 64));
    folderView->setGridSize(QSize(120, 110)); // Gleichmäßige Kacheln für Icon+Text
    folderView->setResizeMode(QListView::Adjust);
    folderView->setUniformItemSizes(true);
    folderView->setSpacing(10);
    
    // Initialisiere den Thumbnail-Generator
    m_thumbnailGenerator = new ThumbnailGenerator(this);
    
    // Verbinde Signale des Thumbnail-Generators
    connect(m_thumbnailGenerator, &ThumbnailGenerator::thumbnailGenerationStarted,
            this, &MainWindow::onThumbnailGenerationStarted);
    connect(m_thumbnailGenerator, &ThumbnailGenerator::thumbnailGenerationProgress,
            this, &MainWindow::onThumbnailGenerationProgress);
    connect(m_thumbnailGenerator, &ThumbnailGenerator::thumbnailGenerationFinished,
            this, &MainWindow::onThumbnailGenerationFinished);
    
    // Debug-Ausgabe
    qDebug() << "ThumbnailDelegate installiert und View konfiguriert";

    // NameMaxLength aus program-settings.json auslesen
    int nameMaxLength = 10;
    QString settingsPath = QCoreApplication::applicationDirPath() + "/program-settings.json";
    QFile settingsFile(settingsPath);
    if(settingsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
        QJsonObject obj = doc.object();
        if(obj.contains("NameMaxLength")) {
            nameMaxLength = obj.value("NameMaxLength").toInt(10);
        }


    }


    // Delegate nur für folderContentView setzen
    if(folderContentView)
        folderContentView->setItemDelegate(new NameShortenDelegate(nameMaxLength, folderContentView));

    // Toolbar oben im linken Panel
    QToolBar *leftToolBar = new QToolBar(leftPanel);
    leftToolBar->setMovable(false);
    leftToolBar->setFloatable(false);
    leftToolBar->setIconSize(QSize(20,20));
    // Toolbar in Layout einfügen
    if(leftLayout) {
        leftLayout->insertWidget(0, leftToolBar);
    }



    // Signal für Laufwerkswechsel
    connect(driveComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onDriveChanged);
    folderView->setFlow(QListView::LeftToRight);
    
    // Aktualisiere die Anzeige - wichtig für Thumbnails
    folderView->viewport()->update();
    qDebug() << "View-Einstellungen aktualisiert";
    folderView->setResizeMode(QListView::Adjust);
    folderView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderView->setMinimumWidth(180);

    // Signal: Datei im Ordnerbaum geklickt
    connect(folderView, &QListView::clicked, this, &MainWindow::onFolderSelected);
    connect(folderView, &QListView::doubleClicked, this, &MainWindow::onFolderDoubleClicked);
    // Thumbnails im linken Panel anzeigen
    folderView->setItemDelegate(new ThumbnailDelegate(folderView));

    // Initialisiere Pfad auf erstes Laufwerk
    if (driveComboBox->count() > 0) {
        QString drivePath = driveComboBox->itemText(0);
        QModelIndex rootIndex = folderModel->index(drivePath);
        folderView->setRootIndex(rootIndex);
        pathEdit->setText(drivePath);
        // Add initial path to history
        navigationHistory.append(drivePath);
        historyIndex = 0;
        backButton->setEnabled(false);
        forwardButton->setEnabled(false);
        generateThumbnailsForCurrentFolder();
    }



    // Rechtes Panel: Neue MediaViewerPanel-Komponente
    mediaPanel = new MediaViewerPanel();

    // Toolbar für rechtes Panel (unten über der Statusbar)
    QToolBar* rightToolBar = new QToolBar();
    rightToolBar->setMovable(false);
    rightToolBar->setFloatable(false);
    rightToolBar->setIconSize(QSize(32,32));
    
    // Lade externe Programme aus den Einstellungen
    QString imageEditorPath = "C:/Program Files/IrfanView/i_view64.exe";
    QString videoPlayerPath = "C:/Program Files/MPC-HC/mpc-hc64.exe";
    QString fileInfoUrlBase = "https://fetlife.com/";
    
    // Versuche, die Pfade aus der Konfigurationsdatei zu laden
    QFile configFile2("build/program-settings.json");
    if(configFile2.open(QIODevice::ReadOnly)) {
        QJsonDocument configDoc = QJsonDocument::fromJson(configFile2.readAll());
        QJsonObject configObj = configDoc.object();
        
        // Externe Programme laden
        if(configObj.contains("externalPrograms")) {
            QJsonObject extPrograms = configObj["externalPrograms"].toObject();
            if(extPrograms.contains("imageEditor"))
                imageEditorPath = extPrograms["imageEditor"].toString();
            if(extPrograms.contains("videoPlayer"))
                videoPlayerPath = extPrograms["videoPlayer"].toString();
        }
        
        // URL-Basis für Browser laden
        if(configObj.contains("fileInfoUrlBase"))
            fileInfoUrlBase = configObj["fileInfoUrlBase"].toString();
            
        configFile2.close();
    }
    
    // Erstelle Actions für externe Programme
    QAction* openInIrfanView = new QAction(QIcon::fromTheme("image-x-generic"), "In Honeyview öffnen", this);
    QAction* openInMpcHc = new QAction(QIcon::fromTheme("video-x-generic"), "In MPC-HC öffnen", this);
    QAction* openInBrowser = new QAction(QIcon::fromTheme("web-browser"), "Im Browser öffnen", this);
    QAction* openInFolder = new QAction(QIcon::fromTheme("folder-open"), "Im Explorer anzeigen", this);
    
    // Verbinde mit Slots zum Öffnen der externen Programme
    connect(openInIrfanView, &QAction::triggered, this, [=]() {
        QString filePath = folderModel->filePath(folderView->currentIndex());
        if(!filePath.isEmpty() && QFile::exists(filePath)) {
            QProcess::startDetached(imageEditorPath, QStringList() << filePath);
        }
    });
    
    connect(openInMpcHc, &QAction::triggered, this, [=]() {
        QString filePath = folderModel->filePath(folderView->currentIndex());
        if(!filePath.isEmpty() && QFile::exists(filePath)) {
            QProcess::startDetached(videoPlayerPath, QStringList() << filePath);
        }
    });
    
    connect(openInBrowser, &QAction::triggered, this, [=]() {
        QString filePath = folderModel->filePath(folderView->currentIndex());
        if(!filePath.isEmpty() && QFile::exists(filePath)) {
            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.fileName();
            
            // Extrahiere den Teil nach dem letzten Unterstrich
            QString idPart;
            int lastUnderscorePos = fileName.lastIndexOf('_');
            
            if (lastUnderscorePos != -1 && lastUnderscorePos < fileName.length() - 1) {
                // Extrahiere alles nach dem letzten Unterstrich
                idPart = fileName.mid(lastUnderscorePos + 1);
                
                // Entferne die Dateiendung, falls vorhanden
                int dotPos = idPart.lastIndexOf('.');
                if (dotPos != -1) {
                    idPart = idPart.left(dotPos);
                }
            } else {
                // Wenn kein Unterstrich gefunden wurde, verwende den ganzen Dateinamen ohne Endung
                idPart = fileName;
                int dotPos = idPart.lastIndexOf('.');
                if (dotPos != -1) {
                    idPart = idPart.left(dotPos);
                }
            }
            
            QString url = fileInfoUrlBase + idPart;
            QDesktopServices::openUrl(QUrl(url));
        }
    });
    
    connect(openInFolder, &QAction::triggered, this, [=]() {
        QString filePath = folderModel->filePath(folderView->currentIndex());
        if(!filePath.isEmpty() && QFile::exists(filePath)) {
            QString nativePath = QDir::toNativeSeparators(filePath);
            QProcess::startDetached("explorer.exe", QStringList() << "/select," + nativePath);
        }
    });
    
    rightToolBar->addAction(openInIrfanView);
    rightToolBar->addAction(openInMpcHc);
    rightToolBar->addAction(openInBrowser);
    rightToolBar->addAction(openInFolder);
    // Container-Widget für die Toolbar am unteren Rand
    QWidget* rightToolbarContainer = new QWidget();
    QHBoxLayout* rightToolbarLayout = new QHBoxLayout(rightToolbarContainer);
    rightToolbarLayout->setContentsMargins(0,0,0,0);
    rightToolbarLayout->setSpacing(0);
    rightToolbarLayout->addStretch();
    rightToolbarLayout->addWidget(rightToolBar);
    rightToolbarLayout->addStretch();
    // Haupt-Container für das rechte Panel
    QWidget* rightPanelContainer = new QWidget();
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(rightPanelContainer);
    rightPanelLayout->setContentsMargins(10,10,10,10);
    rightPanelLayout->setSpacing(0);
    rightPanelLayout->addWidget(mediaPanel, 1); // MediaViewerPanel nimmt den ganzen Platz
    rightPanelLayout->addWidget(rightToolbarContainer, 0);





    // Infofenster oben rechts, halbtransparent


    

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanelContainer);
    mainSplitter->setHandleWidth(20);
    mainSplitter->setStyleSheet("QSplitter::handle { background: #f0f0f0; border-radius: 3px; }");
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setCollapsible(0, false);
    mainSplitter->setCollapsible(1, false);
    mainSplitter->setSizes({220, 600});

    setCentralWidget(mainSplitter);
    statusBar();

    // Fenster- und Splitter-Positionen erst jetzt laden
    SettingsManager::loadWindowSettings(this, mainSplitter, pathEdit);
    
    // Wenn ein Pfad geladen wurde, setze Root-Index und generiere Thumbnails
    if (!pathEdit->text().isEmpty()) {
        QModelIndex rootIndex = folderModel->index(pathEdit->text());
        folderView->setRootIndex(rootIndex);
        generateThumbnailsForCurrentFolder();
    }
}



MainWindow::~MainWindow()
{
    // Kein explizites Speichern mehr hier, das passiert jetzt nur noch im closeEvent.
}



void MainWindow::onFolderViewContextMenu(const QPoint& pos)
{
    QModelIndexList selected = folderView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;
    
    QMenu menu(this);
    QAction *resizeAction = menu.addAction("Resize (Topaz Gigapixel AI)");
    
    // Neue Option zum Löschen aller Thumbnails
    QAction *clearThumbsAction = menu.addAction("Alle Thumbnails löschen");
    
    QAction *chosen = menu.exec(folderView->viewport()->mapToGlobal(pos));
    
    if (chosen == resizeAction) {
        QStringList files;
        for (const QModelIndex &idx : selected) {
            if (folderModel->isDir(idx)) continue;
            files << folderModel->filePath(idx);
        }

        // Externe Resize-Funktionalität aufrufen
        ExternalTools::startResizeExternal(files);
    } 
    else if (chosen == clearThumbsAction) {
        // Thumbnails löschen
        clearAllThumbnails();
    }
}



// Die Methode startResizeExternal() wurde in ExternalTools ausgelagert



void MainWindow::onDriveChanged(int index)
{
    if (index < 0) return;
    QString drivePath = driveComboBox->itemText(index);
    QModelIndex rootIndex = folderModel->index(drivePath);
    folderView->setRootIndex(rootIndex);
    pathEdit->setText(drivePath);
    SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit); // <-- Speichern nach Verzeichniswechsel
    // Manage history
    while (navigationHistory.size() > historyIndex + 1)
        navigationHistory.removeLast();
    navigationHistory.append(drivePath);
    historyIndex++;
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
}

void MainWindow::onFolderDoubleClicked(const QModelIndex &index)
{
    // Vor dem Öffnen eines Ordners Thumbnails generieren
    generateThumbnailsForCurrentFolder();

    QString filePath = folderModel->filePath(index);
    QFileInfo info(filePath);
    if (info.isDir()) {
        folderView->setRootIndex(index);
        pathEdit->setText(filePath);
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit); // <-- Speichern nach Verzeichniswechsel
        // Manage history
        while (navigationHistory.size() > historyIndex + 1)
            navigationHistory.removeLast();
        navigationHistory.append(filePath);
        historyIndex++;
        backButton->setEnabled(historyIndex > 0);
        forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    } else {
        // Optional: Bei Datei Doppelklick kann geöffnet werden
        onFolderSelected(index);
    }
}



void MainWindow::onFolderSelected(const QModelIndex &index)
{
    // Thumbnails für das aktuelle Verzeichnis generieren
    generateThumbnailsForCurrentFolder();

    QString filePath = folderModel->filePath(index);
    QFileInfo info(filePath);
    if (info.isDir()) {
        // Beim Klick auf Ordner passiert nichts besonderes
        return;
    }

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filePath);

    // Medien-Dateitypen (Bild oder Video) werden von MediaViewerPanel behandelt
    if (type.name().startsWith("image/") || 
        type.name().startsWith("video/") || 
        filePath.endsWith(".mp4", Qt::CaseInsensitive) ||
        filePath.endsWith(".avi", Qt::CaseInsensitive) ||
        filePath.endsWith(".mkv", Qt::CaseInsensitive) ||
        filePath.endsWith(".mov", Qt::CaseInsensitive) ||
        filePath.endsWith(".flv", Qt::CaseInsensitive)) {
        
        // Medientyp erkennen und entsprechend anzeigen
        mediaPanel->loadFile(filePath);
    }
    else {
        // Unbekannter Dateityp - mit Standardprogramm öffnen
        ExternalTools::openWithDefaultProgram(filePath);
    }
}

// ...

void MainWindow::resizeEvent(QResizeEvent *event)
{
    // Die Größenänderung wird automatisch vom MediaViewerPanel behandelt
    QMainWindow::resizeEvent(event);
}

// ...

void MainWindow::wheelEvent(QWheelEvent *event)
{
    // Zoom mit CTRL + Mausrad an MediaViewerPanel delegieren
    if (mediaPanel && (event->modifiers() & Qt::ControlModifier)) {
        QPoint numDegrees = event->angleDelta() / 8;
        QPoint numSteps = numDegrees / 15;
        
        if (!numSteps.isNull()) {
            // Zoom-Faktor berechnen
            double factor = (numSteps.y() > 0) ? 1.25 : 0.8; // Rein- oder rauszoomen
            
            // Zoom durchführen
            mediaPanel->zoomImage(factor);
            event->accept();
            return;
        }
    }
    
    QMainWindow::wheelEvent(event);
}

// ...

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Datei öffnen"),
        QString(), tr("Bilder/Videos (*.png *.jpg *.jpeg *.bmp *.mp4 *.avi *.mkv *.mov)"));

    if (fileName.isEmpty())
        return;

    mediaPanel->loadFile(fileName);
}

// ...

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Fenstereinstellungen beim Schließen speichern
    SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    QMainWindow::closeEvent(event);
}
void MainWindow::generateThumbnailsForCurrentFolder()
{
    if (!folderView || !folderModel) {
        qDebug() << "folderView ist null?" << (!folderView);
        qDebug() << "folderModel ist null?" << (!folderModel);
        return;
    }

    QModelIndex rootIdx = folderView->rootIndex();
    QString path = folderModel->filePath(rootIdx);

    qDebug() << "Aktueller Pfad:" << path;

    // Starte die asynchrone Thumbnail-Generierung
    if (m_thumbnailGenerator && m_thumbnailDelegate) {
        m_thumbnailGenerator->startThumbnailGeneration(folderView, folderModel, m_thumbnailDelegate);
    }
}

void MainWindow::clearAllThumbnails()
{
    // Thumbnail-Generierung stoppen, falls sie läuft
    if (m_thumbnailGenerator && m_thumbnailGenerator->isGenerating()) {
        m_thumbnailGenerator->stopThumbnailGeneration();
    }
    
    // Thumbnails-Verzeichnis ermitteln
    QString thumbDir = QCoreApplication::applicationDirPath() + "/thumbnails";
    
    // Alle Dateien im Thumbnails-Ordner löschen
    QDir dir(thumbDir);
    if (dir.exists()) {
        qDebug() << "Lösche alle Thumbnails in:" << thumbDir;
        
        // Alle PNG-Dateien löschen
        QStringList filters;
        filters << "*_thumb.png";
        dir.setNameFilters(filters);
        
        int count = 0;
        foreach (QString file, dir.entryList()) {
            if (dir.remove(file)) {
                count++;
            }
        }
        
        QMessageBox::information(this, "Thumbnails gelöscht", 
                               QString("%1 Thumbnails wurden gelöscht.").arg(count));
        
        // Aktuelle Ansicht aktualisieren und neue Thumbnails generieren
        folderView->viewport()->update();
        generateThumbnailsForCurrentFolder();
    }
}

// Slots für den ThumbnailGenerator
void MainWindow::onThumbnailGenerationStarted()
{
    // Statusbar-Nachricht oder andere UI-Anzeige
    statusBar()->showMessage(tr("Generiere Thumbnails..."));
}

void MainWindow::onThumbnailGenerationProgress(int percent)
{
    // Update der Statusbar mit Fortschritt
    statusBar()->showMessage(tr("Generiere Thumbnails: %1%").arg(percent));
}

void MainWindow::onThumbnailGenerationFinished()
{
    // Generierung abgeschlossen
    statusBar()->showMessage(tr("Thumbnail-Generierung abgeschlossen"), 3000);
    
    // Aktualisiere die Ansicht
    if (folderView) {
        folderView->viewport()->update();
    }
}

// Slot für Zurück-Button
void MainWindow::onBackButtonClicked()
{
    if (historyIndex <= 0) return;
    historyIndex--;
    QString prevPath = navigationHistory.at(historyIndex);
    QModelIndex prevIndex = folderModel->index(prevPath);
    if (prevIndex.isValid()) {
        folderView->setRootIndex(prevIndex);
        pathEdit->setText(prevPath);
        generateThumbnailsForCurrentFolder();
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    }
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
}

// Slot für Vorwärts-Button
void MainWindow::onForwardButtonClicked()
{
    if (historyIndex >= navigationHistory.size() - 1) return;
    historyIndex++;
    QString nextPath = navigationHistory.at(historyIndex);
    QModelIndex nextIndex = folderModel->index(nextPath);
    if (nextIndex.isValid()) {
        folderView->setRootIndex(nextIndex);
        pathEdit->setText(nextPath);
        generateThumbnailsForCurrentFolder();
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    }
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
}

// Slot: Navigiert einen Ordner nach oben
void MainWindow::onUpButtonClicked() {
    QString currentPath = pathEdit->text();
    QDir dir(currentPath);
    if (dir.cdUp()) {
        QString parentPath = dir.absolutePath();
        QModelIndex idx = folderModel->index(parentPath);
        folderView->setRootIndex(idx);
        pathEdit->setText(parentPath);
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
        generateThumbnailsForCurrentFolder();
        // Verlauf verwalten
        while (navigationHistory.size() > historyIndex + 1)
            navigationHistory.removeLast();
        navigationHistory.append(parentPath);
        historyIndex++;
        backButton->setEnabled(historyIndex > 0);
        forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    }
}

// Überschreibe Event-Filter für Mausrad-Events
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == folderView->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *we = static_cast<QWheelEvent*>(event);
        QPoint pos = we->position().toPoint();
        int half = folderView->width() / 2;
        if (pos.x() > half) {
            int delta = we->angleDelta().y();
            int step = (delta > 0) ? -1 : 1;
            QModelIndex current = folderView->currentIndex();
            int row = current.isValid() ? current.row() : 0;
            int maxRow = folderModel->rowCount(folderView->rootIndex()) - 1;
            int newRow = qBound(0, row + step, maxRow);
            QModelIndex newIndex = folderModel->index(newRow, 0, folderView->rootIndex());
            folderView->setCurrentIndex(newIndex);
            // Datei im rechten Panel anzeigen
            onFolderSelected(newIndex);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
