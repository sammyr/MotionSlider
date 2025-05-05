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
#include <QPushButton>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QDir>
#include <QPainter>
#include <QColor>
#include <QStringList> // Für Navigation History
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QEvent>
#include <QResizeEvent>
#include <QClipboard>
#include <QMimeData>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>
#include "ScrollConfig.h"
#include "about.h"

#include "NameShortenDelegate.h"
#include "ThumbnailDelegate.h"
#include "ThumbnailGenerator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "ContextMenuManager.h"
#include "FileOperations.h"
#include "ScrollNavigator.h"
#include <QImageReader>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/icons/appicon.ico")); // Icon aus Qt-Resource setzen
    setWindowTitle("Bildblick");

    // Menüleiste: Datei-Menü zuerst, Hilfe-Menü an zweiter Stelle
    QMenu* fileMenu = menuBar()->addMenu(tr("&Datei"));
    fileMenu->addAction(tr("&Öffnen..."), this, &MainWindow::openFile);
    QMenu* helpMenu = menuBar()->addMenu(tr("&Hilfe"));
    QAction* aboutAction = helpMenu->addAction(tr("Über ..."));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // Initialisiere optionale Views
    folderContentView = nullptr;
    folderContentModel = nullptr;

    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setFormat("Generiere Thumbnails: %p%");
    progressBar->setVisible(false);
    statusBar()->addPermanentWidget(progressBar);
    historyIndex = -1;
    // Menü bleibt: Datei zuerst, Hilfe danach.

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
    upButton->setIconSize(QSize(40,40));
    upButton->setFixedSize(50,50);
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
    
    // Einstellungen laden
    QJsonObject configObj;
    QFile cfgFile("build/program-settings.json");
    if (cfgFile.open(QIODevice::ReadOnly)) {
        configObj = QJsonDocument::fromJson(cfgFile.readAll()).object();
        cfgFile.close();
    }

    // Aktuellen Sortierwert aus den Einstellungen laden
    QString currentSort = "name"; // Standardwert
    if(configObj.contains("sortOrder"))
        currentSort = configObj["sortOrder"].toString();
            
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
    
    // Thumbnail-Größe aus Settings laden
    int currentThumbSize = 160;
    if (configObj.contains("thumbnailSpacing"))
        this->thumbnailSpacing = configObj["thumbnailSpacing"].toInt();
    else
        this->thumbnailSpacing = 24;
    qDebug() << "[DEBUG] thumbnailSpacing aus Settings geladen:" << this->thumbnailSpacing;
    if (configObj.contains("thumbnailSize"))
        currentThumbSize = configObj["thumbnailSize"].toInt();
    thumbnailSize = currentThumbSize;
    // SpinBox für Thumbnail-Größe
    thumbnailSizeSpinBox = new QSpinBox;
    thumbnailSizeSpinBox->setRange(64, 384);
    thumbnailSizeSpinBox->setSingleStep(16);
    thumbnailSizeSpinBox->setValue(currentThumbSize);
    thumbnailSizeSpinBox->setToolTip(tr("Thumbnail-Größe"));
    toolbarLayout->addWidget(new QLabel(tr("Größe:")), 0);
    toolbarLayout->addWidget(thumbnailSizeSpinBox);
    connect(thumbnailSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onThumbnailSizeChanged);
    
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
    m_scrollNav = new ScrollNavigator(folderView, folderModel);
    // Zeige Bild im rechten Panel auch bei Tastatur-Navigation
    connect(folderView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onFolderSelected);

    // Verzeichnis-Wechsel aktivieren (nach folderView Instanziierung)
    connect(driveComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onDriveChanged);
    connect(folderView, &QListView::doubleClicked, this, &MainWindow::onFolderDoubleClicked);
    connect(folderView, &QListView::clicked, this, &MainWindow::onFolderSelected);

    // Setze benutzerdefinierten Delegate für Thumbnails und konfiguriere View
    // Maximale Namenslänge aus Einstellungen lesen
    int nameMaxLength = 10;
    if (configObj.contains("NameMaxLength"))
        nameMaxLength = configObj["NameMaxLength"].toInt();

    m_thumbnailDelegate = new ThumbnailDelegate(nameMaxLength, folderView);
    folderView->setItemDelegate(m_thumbnailDelegate);
    folderView->setViewMode(QListView::IconMode);
    folderView->setIconSize(QSize(thumbnailSize, thumbnailSize));

    updateThumbnailGridSize();
    folderView->setResizeMode(QListView::Adjust);
    folderView->setUniformItemSizes(true);
    folderView->setSpacing(thumbnailSpacing);

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

    // NameMaxLength aus Settings verwenden für folderContentView
    int nameMaxLengthContent = 20;
    if (configObj.contains("NameMaxLength"))
        nameMaxLengthContent = configObj["NameMaxLength"].toInt();
    if (folderContentView)
        folderContentView->setItemDelegate(new NameShortenDelegate(nameMaxLengthContent, folderContentView));

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
    folderView->setFlow(QListView::LeftToRight);
    
    // Aktualisiere die Anzeige - wichtig für Thumbnails
    folderView->viewport()->update();
    qDebug() << "View-Einstellungen aktualisiert";
    folderView->setResizeMode(QListView::Adjust);
    folderView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    folderView->setMinimumWidth(180);

    // Signal: Datei im Ordnerbaum geklickt
    //connect(folderView, &QListView::clicked, this, &MainWindow::onFolderSelected);
    //connect(folderView, &QListView::doubleClicked, this, &MainWindow::onFolderDoubleClicked);
    // Thumbnails im linken Panel anzeigen
    //folderView->setItemDelegate(new ThumbnailDelegate(folderView));

    // Initialisiere Pfad auf erstes Laufwerk
    if (driveComboBox->count() > 0) {
        QString drivePath = driveComboBox->itemText(0);
        folderModel->setRootPath(drivePath);
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

    if (!pathEdit->text().isEmpty()) {
        folderModel->setRootPath(pathEdit->text());
        QModelIndex rootIndex = folderModel->index(pathEdit->text());
        folderView->setRootIndex(rootIndex);
        generateThumbnailsForCurrentFolder();
        // Initiale Auswahl auf erstes Element setzen
        if (folderModel->rowCount(rootIndex) > 0) {
            QModelIndex firstChild = folderModel->index(0, 0, rootIndex);
            folderView->setCurrentIndex(firstChild);
        }
    }

    // Rechtes Panel: Neue MediaViewerPanel-Komponente
    mediaPanel = new MediaViewerPanel();

    // Toolbar für rechtes Panel (unten über der Statusbar)

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
    QAction* openInFolder = new QAction(QIcon::fromTheme("folder-open"), "Im Explorer anzeigen", this);

    connect(openInFolder, &QAction::triggered, this, [=]() {
        QString filePath = folderModel->filePath(folderView->currentIndex());
        if(!filePath.isEmpty() && QFile::exists(filePath)) {
            QString nativePath = QDir::toNativeSeparators(filePath);
            QProcess::startDetached("explorer.exe", QStringList() << "/select," + nativePath);
        }
    });
    



    // Haupt-Container für das rechte Panel
    QWidget* rightPanelContainer = new QWidget();
    QVBoxLayout* rightPanelLayout = new QVBoxLayout(rightPanelContainer);
    rightPanelLayout->setContentsMargins(10,10,10,10);
    rightPanelLayout->setSpacing(0);
    rightPanelLayout->addWidget(mediaPanel, 1); // MediaViewerPanel nimmt den ganzen Platz





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
    // Reagiere auf Panel-Resize
    connect(mainSplitter, &QSplitter::splitterMoved, this, [this](int pos, int index){
        updateThumbnailGridSize();
        // Speichere neue Splitter-Position
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    });

    setCentralWidget(mainSplitter);
    statusBar();
    
    // Fenster- und Splitter-Positionen erst jetzt laden
    SettingsManager::loadWindowSettings(this, mainSplitter, pathEdit);
    
    // Wenn ein Pfad geladen wurde, setze Root-Index und generiere Thumbnails
    if (!pathEdit->text().isEmpty()) {
        folderModel->setRootPath(pathEdit->text());
        QModelIndex rootIndex = folderModel->index(pathEdit->text());
        folderView->setRootIndex(rootIndex);
        generateThumbnailsForCurrentFolder();
    }
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Datei öffnen"), QString(),
        tr("Bilder/Videos (*.png *.jpg *.jpeg *.bmp *.mp4 *.avi *.mkv *.mov)"));
    if (fileName.isEmpty()) return;
    mediaPanel->loadFile(fileName);
}

void MainWindow::clearAllThumbnails() {
    if (m_thumbnailGenerator && m_thumbnailGenerator->isGenerating())
        m_thumbnailGenerator->stopThumbnailGeneration();
    QString thumbDir = QCoreApplication::applicationDirPath() + "/thumbnails";
    QDir dir(thumbDir);
    if (dir.exists()) dir.removeRecursively();
    folderView->viewport()->update();
}

void MainWindow::onThumbnailGenerationStarted() {
    progressBar->setValue(0);
    progressBar->setFormat(tr("Generiere Thumbnails: %p%"));
    progressBar->setVisible(true);
    statusBar()->clearMessage();
}

void MainWindow::onThumbnailGenerationProgress(int percent) {
    progressBar->setValue(percent);
}

void MainWindow::onThumbnailGenerationFinished() {
    progressBar->setValue(100);
    progressBar->setVisible(false);
    statusBar()->showMessage(tr("Thumbnail-Generierung abgeschlossen"), 3000);
    folderView->viewport()->update();
}

void MainWindow::onBackButtonClicked() {
    if (historyIndex <= 0) return;
    historyIndex--;
    QString path = navigationHistory.at(historyIndex);
    folderModel->setRootPath(path);
    folderView->setRootIndex(folderModel->index(path));
    pathEdit->setText(path);
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    generateThumbnailsForCurrentFolder();
}

void MainWindow::onForwardButtonClicked() {
    if (historyIndex >= navigationHistory.size() - 1) return;
    historyIndex++;
    QString path = navigationHistory.at(historyIndex);
    folderModel->setRootPath(path);
    folderView->setRootIndex(folderModel->index(path));
    pathEdit->setText(path);
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    generateThumbnailsForCurrentFolder();
}

void MainWindow::onUpButtonClicked() {
    QString currentPath = pathEdit->text();
    QDir dir(currentPath);
    if (dir.cdUp()) {
        QString parent = dir.absolutePath();
        folderModel->setRootPath(parent);
        QModelIndex rootIndex = folderModel->index(parent);
        folderView->setRootIndex(rootIndex);
        pathEdit->setText(parent);
        generateThumbnailsForCurrentFolder();
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
        // update history
        while (navigationHistory.size() > historyIndex + 1) navigationHistory.removeLast();
        navigationHistory.append(parent);
        historyIndex++;
        backButton->setEnabled(historyIndex > 0);
        forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    }
}

void MainWindow::onFolderViewContextMenu(const QPoint &pos) {
    QModelIndexList sel = folderView->selectionModel()->selectedIndexes();
    QMenu menu(this);
    ContextMenuManager::populateFolderContextMenu(&menu, sel, folderModel, this, folderView);
    menu.exec(folderView->viewport()->mapToGlobal(pos));
}

void MainWindow::copyItems() {
    QModelIndexList sel = folderView->selectionModel()->selectedIndexes();
    QMimeData *md = new QMimeData;
    QList<QUrl> urls;
    for (auto &idx : sel) {
        if (idx.column() != 0) continue;
        QString path = folderModel->filePath(idx);
        urls.append(QUrl::fromLocalFile(path));
    }
    md->setUrls(urls);
    QApplication::clipboard()->setMimeData(md);
}

void MainWindow::cutItems() {
    FileOperations::cutItems(folderView, folderModel, cutPaths, cutOperationActive);
}

void MainWindow::deleteItems() {
    FileOperations::deleteItems(folderView, folderModel);
}

void MainWindow::showProperties() {
    FileOperations::showProperties(this, folderView, folderModel);
}

void MainWindow::onThumbnailSizeChanged(int size) {
    thumbnailSize = size;
    folderView->setIconSize(QSize(size, size));
    updateThumbnailGridSize();
    // Settings schreiben
    QFile cfg("build/program-settings.json");
    if (cfg.open(QIODevice::ReadWrite)) {
        QJsonDocument doc = QJsonDocument::fromJson(cfg.readAll());
        QJsonObject obj = doc.object();
        obj["thumbnailSize"] = size;
        cfg.resize(0);
        cfg.write(QJsonDocument(obj).toJson());
        cfg.close();
    }
}

// Implementierung Verzeichnis-Wechsel und Thumbnail-Generierung
MainWindow::~MainWindow() {}

void MainWindow::onDriveChanged(int index) {
    if (index < 0) return;
    QString drivePath = driveComboBox->itemText(index);
    folderModel->setRootPath(drivePath);
    QModelIndex rootIndex = folderModel->index(drivePath);
    folderView->setRootIndex(rootIndex);
    pathEdit->setText(drivePath);
    SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    // Verlauf
    while (navigationHistory.size() > historyIndex + 1) navigationHistory.removeLast();
    navigationHistory.append(drivePath);
    historyIndex++;
    backButton->setEnabled(historyIndex > 0);
    forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
    generateThumbnailsForCurrentFolder();
}

void MainWindow::onFolderDoubleClicked(const QModelIndex &index) {
    QString filePath = folderModel->filePath(index);
    QFileInfo info(filePath);
    if (info.isDir()) {
        folderModel->setRootPath(filePath);
        QModelIndex dirIndex = folderModel->index(filePath);
        folderView->setRootIndex(dirIndex);
        pathEdit->setText(filePath);
        SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
        while (navigationHistory.size() > historyIndex + 1) navigationHistory.removeLast();
        navigationHistory.append(filePath);
        historyIndex++;
        backButton->setEnabled(historyIndex > 0);
        forwardButton->setEnabled(historyIndex < navigationHistory.size() - 1);
        // Thumbnails für das neue Verzeichnis generieren
        generateThumbnailsForCurrentFolder();
    } else {
        mediaPanel->loadFile(filePath);
    }
}

void MainWindow::onFolderSelected(const QModelIndex &index) {
    QString filePath = folderModel->filePath(index);
    QFileInfo info(filePath);
    if (!info.isDir()) {
        mediaPanel->loadFile(filePath);
        // Statusleiste: Dateiname, Abmessungen und Größe
        QString fileName = info.fileName();
        double sizeMB = info.size() / (1024.0 * 1024.0);
        QString sizeMbStr = QString::number(sizeMB, 'f', 2) + " MB";
        QImageReader reader(filePath);
        QSize imgSize = reader.size();
        QString dimStr = imgSize.isValid() ? QString("%1×%2 px").arg(imgSize.width()).arg(imgSize.height()) : QString();
        statusBar()->showMessage(QString("%1, %2, %3").arg(fileName, dimStr, sizeMbStr));
    } else {
        mediaPanel->clearMedia();
        statusBar()->clearMessage();
    }
}

void MainWindow::generateThumbnailsForCurrentFolder() {
    if (!folderView || !folderModel) return;
    QModelIndex rootIdx = folderView->rootIndex();
    QString path = folderModel->filePath(rootIdx);
    if (m_thumbnailGenerator && m_thumbnailDelegate) {
        if (m_thumbnailGenerator->isGenerating()) {
            m_thumbnailGenerator->stopThumbnailGeneration();
        }
        m_thumbnailGenerator->startThumbnailGeneration(folderView, folderModel, m_thumbnailDelegate);
    }
}

// Dynamische Anpassung der Thumbnail-GridSize
void MainWindow::updateThumbnailGridSize() {
    if (!folderView) return;
    int nameMaxLength = m_thumbnailDelegate ? m_thumbnailDelegate->getMaxLength() : 10;
    QFontMetrics fm(folderView->font());
    int textWidth = fm.horizontalAdvance(QString(nameMaxLength, 'W'));
    int gridWidth = thumbnailSize; // Nur die Thumbnail-Größe bestimmt die Breite
    int gridHeight = thumbnailSize + fm.height() * 2 + 8; // Noch weniger Abstand
    folderView->setGridSize(QSize(gridWidth, gridHeight));
    folderView->setSpacing(thumbnailSpacing); // Abstand aus Settings
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateThumbnailGridSize();
}

void MainWindow::wheelEvent(QWheelEvent *event) {
    QMainWindow::wheelEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Fenster- und Splitter-Einstellungen speichern
    SettingsManager::saveWindowSettings(this, mainSplitter, pathEdit);
    QMainWindow::closeEvent(event);
}

void MainWindow::pasteItems() {
    FileOperations::pasteItems(folderView, folderModel, cutOperationActive, cutPaths);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showAboutDialog() {
    AboutDialog dlg(this);
    dlg.exec();
}
