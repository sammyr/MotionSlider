#pragma once

#include <QMenu>
#include <QModelIndexList>
#include <QFileSystemModel>
#include <QAbstractItemView>
#include <QString>

class MainWindow; // Vorwärts-Deklaration

namespace ContextMenuManager {
    // Füllt das Kontextmenü für das Verzeichnis-View
    void populateFolderContextMenu(QMenu *menu,
                                   const QModelIndexList &sel,
                                   QFileSystemModel *model,
                                   MainWindow *win,
                                   QAbstractItemView *view);
}
