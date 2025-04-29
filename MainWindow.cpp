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

#include "NameShortenDelegate.h"
#include "ThumbnailDelegate.h"
#include "ThumbnailGenerator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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
    driveComboBox = new QComboBox;
    // Laufwerke eintragen
    const auto drives = QDir::drives();
    for (const QFileInfo &drive : drives) {
        driveComboBox->addItem(drive.absoluteFilePath());
    }


    toolbarLayout->addWidget(driveComboBox);
    upButton = new QToolButton;
    // Ordner-hoch-Icon: Versuche eigenes Icon, sonst fallback auf Windows-Standard
    // Gelbes Ordner-Icon für Up-Button
    QIcon folderUpIcon = style->standardIcon(QStyle::SP_DirOpenIcon);
    upButton->setIcon(folderUpIcon);
    upButton->setToolTip("Eine Ebene nach oben");
    upButton->setAutoRaise(true);
    toolbarLayout->addWidget(upButton);
    pathEdit = new QLineEdit;
    pathEdit->setReadOnly(false);
    toolbarLayout->addWidget(pathEdit, 1);
    // Padding rechts
    toolbarLayout->addSpacing(5);

    leftLayout->addLayout(toolbarLayout);
    connect(upButton, &QToolButton::clicked, this, &MainWindow::onUpButtonClicked);

    folderView = new QListView;
    folderView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(folderView, &QWidget::customContextMenuRequested, this, &MainWindow::onFolderViewContextMenu);
    leftLayout->addWidget(folderView);
    folderModel = new QFileSystemModel(this);
    folderModel->setRootPath("");
    folderView->setModel(folderModel);

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
    QAction *leftDummyAction = new QAction(QIcon::fromTheme("document-new"), "Neu", this);
    leftToolBar->addAction(leftDummyAction);
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
        generateThumbnailsForCurrentFolder();
    }



    // Rechtes Panel: Neue MediaViewerPanel-Komponente
    mediaPanel = new MediaViewerPanel(mainSplitter);




    // Infofenster oben rechts, halbtransparent


    

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(mediaPanel);
    mainSplitter->setHandleWidth(12);
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



void MainWindow::onUpButtonClicked()
{
    QModelIndex currentRoot = folderView->rootIndex();
    QString currentPath = folderModel->filePath(currentRoot);
    QDir dir(currentPath);
    if (dir.isRoot()) return; // Schon im Laufwerks-Root
    dir.cdUp();
    QString upPath = dir.absolutePath();
    QModelIndex upIndex = folderModel->index(upPath);
    if (upIndex.isValid()) {
        folderView->setRootIndex(upIndex);
        pathEdit->setText(upPath);
        generateThumbnailsForCurrentFolder();
        // Laufwerksauswahl ggf. anpassen
        QString drive = upPath.left(3); // z.B. "C:/"
        int driveIdx = driveComboBox->findText(drive, Qt::MatchStartsWith);
        if (driveIdx >= 0) driveComboBox->setCurrentIndex(driveIdx);
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit); // <-- Speichern nach Verzeichniswechsel
    }
}



void MainWindow::onDriveChanged(int index)
{
    if (index < 0) return;
    QString drivePath = driveComboBox->itemText(index);
    QModelIndex rootIndex = folderModel->index(drivePath);
    folderView->setRootIndex(rootIndex);
    pathEdit->setText(drivePath);
    SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit); // <-- Speichern nach Verzeichniswechsel
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
            // Zoom-Faktor anpassen
            double factor = (numSteps.y() > 0) ? 1.25 : 0.8; // Rein- oder rauszoomen
            
            // Zoom an MediaViewerPanel delegieren
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
