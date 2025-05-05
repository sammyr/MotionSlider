#pragma once

#include <QMainWindow>
#include <QProgressBar>
#include <QListView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QSplitter>
#include <QToolBar>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QSpinBox>
#include "FileOperations.h"
#include "ScrollNavigator.h"

// Unsere eigenen Klassen einbinden
#include "ThumbnailDelegate.h"
#include "ThumbnailGenerator.h"
#include "SettingsManager.h"
#include "ExternalTools.h"
#include "MediaViewerPanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override; // Für Qt MOC notwendig (Stubs)
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openFile();

private slots:
    void showAboutDialog();
private:
    QSplitter* mainSplitter;
    MediaViewerPanel* mediaPanel; // Neue Medien-Anzeige-Komponente
    
    // Thumbnail-Generator und Delegate
    ThumbnailGenerator* m_thumbnailGenerator;
    ThumbnailDelegate* m_thumbnailDelegate;
    QProgressBar* progressBar; // Fortschrittsbalken für Statusleiste
    
    // Fenstereinstellungen werden jetzt über den SettingsManager verwaltet

public slots:
    void onFolderViewContextMenu(const QPoint& pos);
    void cutItems();
    void copyItems();
    void deleteItems();
    void showProperties();
    void pasteItems();
    void onThumbnailSizeChanged(int size);

private:
    // Ruft den ThumbnailGenerator auf, um Thumbnails zu erstellen
    void generateThumbnailsForCurrentFolder();

    // Panel für Ordneransicht (links)
    QListView *folderView;
    QFileSystemModel *folderModel;
    QLineEdit *pathEdit;
    QComboBox *driveComboBox; // Laufwerksauswahl
    QToolButton *backButton; // Neuer Zurück-Button
    QToolButton *forwardButton; // Neuer Vorwärts-Button
    QToolButton *upButton; // Gehe einen Ordner hoch
    QStringList navigationHistory; // Verlauf der besuchten Ordner
    int historyIndex; // Aktuelle Position im Verlauf
    int scrollSplitPercent; // Prozentuale Position für Mausrad-Switch im linken Panel
    // Thumbnail-Größe im File Explorer
    QSpinBox *thumbnailSizeSpinBox;
    int thumbnailSize;
    // Cut-Paste Unterstützung
    QStringList cutPaths;    // Zu verschiebende Dateien
    bool cutOperationActive; // Flag, ob aktuell ausgeschnitten wurde
    ScrollNavigator* m_scrollNav; // Event-Filter für Mausrad-Navigation ausgelagert

    // Navigation
    void onFolderDoubleClicked(const QModelIndex &index);
    void onDriveChanged(int index);
    void onBackButtonClicked();
    void onForwardButtonClicked();
    void onUpButtonClicked();

    // Panel für Ordnerinhalt (rechts)
    QListView *folderContentView;
    QFileSystemModel *folderContentModel;
    // Scrollbereich für Mausrad-gestützte Navigation (unteres 25% im linken Panel)
    QWidget *wheelArea;

public slots:
    void onFolderSelected(const QModelIndex &index);
    void clearAllThumbnails();
    
    // Slots für Thumbnail-Generator
    void onThumbnailGenerationStarted();
    void onThumbnailGenerationProgress(int percent);
    void onThumbnailGenerationFinished();
    
private:
    void updateThumbnailGridSize();
    int thumbnailSpacing = 24; // Abstand zwischen Thumbnails (aus Settings)

};
