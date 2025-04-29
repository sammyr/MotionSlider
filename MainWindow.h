#pragma once

#include <QMainWindow>
#include <QListView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QSplitter>
#include <QToolBar>

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

private slots:
    void openFile();

private:
    QSplitter* mainSplitter;
    MediaViewerPanel* mediaPanel; // Neue Medien-Anzeige-Komponente
    
    // Thumbnail-Generator und Delegate
    ThumbnailGenerator* m_thumbnailGenerator;
    ThumbnailDelegate* m_thumbnailDelegate;
    
    // Fenstereinstellungen werden jetzt über den SettingsManager verwaltet

private slots:
    void onFolderViewContextMenu(const QPoint& pos);

private:
    // Ruft den ThumbnailGenerator auf, um Thumbnails zu erstellen
    void generateThumbnailsForCurrentFolder();

    // Panel für Ordneransicht (links)
    QListView *folderView;
    QFileSystemModel *folderModel;
    QLineEdit *pathEdit;
    QToolButton *upButton;
    QComboBox *driveComboBox; // Laufwerksauswahl

    // Navigation
    void onFolderDoubleClicked(const QModelIndex &index);
    void onDriveChanged(int index);
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

};
