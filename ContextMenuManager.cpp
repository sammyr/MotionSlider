#include "ContextMenuManager.h"
#include "MainWindow.h"
#include <QAction>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileSystemModel>
#include <QAbstractItemView>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QLineEdit>

namespace ContextMenuManager {
void populateFolderContextMenu(QMenu *menu,
                               const QModelIndexList &sel,
                               QFileSystemModel *model,
                               MainWindow *win,
                               QAbstractItemView *view) {

    // Öffnen mit Resize-Programm
    QAction *resizeAct = menu->addAction(win->tr("Öffnen mit Resize"));
    QString rPath;
    QFile cfgR("build/program-settings.json");
    if (cfgR.open(QIODevice::ReadOnly)) {
        QJsonObject root = QJsonDocument::fromJson(cfgR.readAll()).object();
        rPath = root.value("externalPrograms").toObject().value("resizeProgram").toString();
        cfgR.close();
    }
    if (rPath.isEmpty()) resizeAct->setEnabled(false);
    QObject::connect(resizeAct, &QAction::triggered, [sel, model, rPath]() {
        for (auto idx : sel) {
            if (idx.column() != 0) continue;
            QString path = model->filePath(idx);
            QProcess::startDetached(rPath, QStringList() << path);
        }
    }); 
                                
    // Öffnen mit Photoshop
    QAction *psAct = menu->addAction(win->tr("Öffnen mit Photoshop"));
    QString psPath;
    QFile cfgPS("build/program-settings.json");
    if (cfgPS.open(QIODevice::ReadOnly)) {
        QJsonObject root = QJsonDocument::fromJson(cfgPS.readAll()).object();
        psPath = root.value("externalPrograms").toObject().value("imageditor2").toString();
        cfgPS.close();
    }
    if (psPath.isEmpty()) psAct->setEnabled(false);
    QObject::connect(psAct, &QAction::triggered, [sel, model, psPath]() {
        for (auto idx : sel) {
            if (idx.column() != 0) continue;
            QString path = model->filePath(idx);
            QProcess::startDetached(psPath, QStringList() << path);
        }
    });

    // Lese externe Programme aus Settings
    QFile cfg("build/program-settings.json");
    QJsonObject confObj;
    QJsonObject extProg;
    if (cfg.open(QIODevice::ReadOnly)) {
        confObj = QJsonDocument::fromJson(cfg.readAll()).object();
        extProg = confObj.value("externalPrograms").toObject();
        cfg.close();
    }
    QString honeyPath = extProg.value("imageEditor").toString();
    QString mpcPath = extProg.value("videoPlayer").toString();

    // Im Browser öffnen (mit fileInfoUrlBase und modifiziertem Key)
    QString fileInfoUrlBase = confObj.value("fileInfoUrlBase").toString();
    QAction *browserAct = new QAction(win->tr("Im Browser öffnen"), menu);
    menu->insertAction(menu->actions().isEmpty() ? nullptr : menu->actions().first(), browserAct); // ganz nach oben
    QObject::connect(browserAct, &QAction::triggered, [sel, model, fileInfoUrlBase]() {
        if (sel.isEmpty() || fileInfoUrlBase.isEmpty()) return;
        QFileInfo info(model->filePath(sel.first()));
        QString base = info.baseName();
        int lastUnderscore = base.lastIndexOf('_');
        QString key = (lastUnderscore > 0) ? base.left(lastUnderscore) : base;
        QDesktopServices::openUrl(QUrl(fileInfoUrlBase + key));
    });

    // In Honeyview öffnen
    QAction *honeyAct = menu->addAction(win->tr("In Honeyview öffnen"));
    QObject::connect(honeyAct, &QAction::triggered, [sel, model, honeyPath]() {
        if (honeyPath.isEmpty() || sel.isEmpty()) return;
        QString path = model->filePath(sel.first());
        QProcess::startDetached(honeyPath, QStringList() << path);
    });

    // In MPC-HC öffnen
    QAction *mpcAct = menu->addAction(win->tr("In MPC-HC öffnen"));
    QObject::connect(mpcAct, &QAction::triggered, [sel, model, mpcPath]() {
        if (mpcPath.isEmpty() || sel.isEmpty()) return;
        QString path = model->filePath(sel.first());
        QProcess::startDetached(mpcPath, QStringList() << path);
    });

    // Im Explorer öffnen
    QAction *openFolderAct = menu->addAction(win->tr("Im Ordner öffnen"));
    QObject::connect(openFolderAct, &QAction::triggered, [sel, model]() {
        for (auto idx : sel) {
            if (idx.column() != 0) continue;
            QString path = model->filePath(idx);
            QString native = QDir::toNativeSeparators(path);
            QProcess::startDetached("explorer.exe", QStringList() << "/select," + native);
            break;
        }
    });
    menu->addSeparator();
    menu->addAction(win->tr("Ausschneiden"), win, &MainWindow::cutItems);
    menu->addAction(win->tr("Kopieren"), win, &MainWindow::copyItems);
    QAction *pasteAct = menu->addAction(win->tr("Einfügen"), win, &MainWindow::pasteItems);
    if (!QApplication::clipboard()->mimeData()->hasUrls()) pasteAct->setEnabled(false);
    // Lösch- und Umbenennen-Menüpunkte
    menu->addAction(win->tr("Löschen"), win, &MainWindow::deleteItems);
    QAction *renameAct = menu->addAction(win->tr("Umbenennen"));
    QObject::connect(renameAct, &QAction::triggered, [sel, view, model, win]() {
        if (sel.isEmpty()) return;
        QModelIndex idx = sel.first();
        if (idx.column() != 0) return;
        QString oldPath = model->filePath(idx);
        QFileInfo info(oldPath);
        QString oldName = info.fileName();
        bool ok;
        QString newName = QInputDialog::getText(win, win->tr("Umbenennen"),
            win->tr("Neuer Name:"), QLineEdit::Normal, oldName, &ok);
        if (!ok || newName.isEmpty() || newName == oldName) return;
        QDir dir = info.dir();
        QString newPath = dir.filePath(newName);
        if (!QFile::rename(oldPath, newPath)) {
            QMessageBox::warning(win, win->tr("Fehler"),
                win->tr("Datei konnte nicht umbenannt werden."));
        } else {
            view->update();
        }
    });
    menu->addAction(win->tr("Eigenschaften"), win, &MainWindow::showProperties);
}
}
