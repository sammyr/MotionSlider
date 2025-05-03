#pragma once

#include <QString>
#include <QStringList>
#include <QFileSystemModel>
#include <QListView>
#include <QWidget>

namespace FileOperations {
    // Rekursive Verzeichnis-Kopie
    bool copyDirRec(const QString &src, const QString &dst);

    // Paste-Funktionalität mit Ausschneiden/Einfügen
    void pasteItems(QListView* view, QFileSystemModel* model, bool &cutOperationActive, QStringList &cutPaths);

    // Cut-Funktionalität
    void cutItems(QListView* view, QFileSystemModel* model, QStringList &cutPaths, bool &cutOperationActive);
    // Delete-Funktionalität
    void deleteItems(QListView* view, QFileSystemModel* model);
    // Anzeige von Eigenschaften
    void showProperties(QWidget* parent, QListView* view, QFileSystemModel* model);
}
