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
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void openFile();

private:
    QSplitter* mainSplitter;
    MediaViewerPanel* mediaPanel; // Neue Medien-Anzeige-Komponente
    
    // Thumbnail-Generator und Delegate
    ThumbnailGenerator* m_thumbnailGenerator;
    ThumbnailDelegate* m_thumbnailDelegate;
    QProgressBar* progressBar; // Fortschrittsbalken für Statusleiste
    
    // Fenstereinstellungen werden jetzt über den SettingsManager verwaltet

private slots:
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

    // Navigation
    void onFolderDoubleClicked(const QModelIndex &index);
    void onDriveChanged(int index);
    void onBackButtonClicked();
    void onForwardButtonClicked();
    void onUpButtonClicked();

    // Panel für Ordnerinhalt (rechts)
    QListView *folderContentView;
    QFileSystemModel *folderContentModel;

private slots:
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
