#include "FileOperations.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QMessageBox>
#include <QModelIndexList>
#include <QListView>
#include <QFileSystemModel>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif
#include <QMediaPlayer>
#include <QEventLoop>
#include <QTime>
#include <QMediaMetaData>

namespace FileOperations {

bool copyDirRec(const QString &src, const QString &dst) {
    QDir srcDir(src);
    if (!QDir().mkpath(dst)) return false;
    for (const QString &d : srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        copyDirRec(srcDir.filePath(d), QDir(dst).filePath(d));
    for (const QString &f : srcDir.entryList(QDir::Files))
        QFile::copy(srcDir.filePath(f), QDir(dst).filePath(f));
    return true;
}

void pasteItems(QListView* view, QFileSystemModel* model, bool &cutOperationActive, QStringList &cutPaths) {
    const QMimeData *md = QApplication::clipboard()->mimeData();
    if (!md->hasUrls()) return;
    QModelIndex root = view->rootIndex();
    QString dest = model->filePath(root);
    for (const QUrl &url : md->urls()) {
        QString src = url.toLocalFile();
        QFileInfo info(src);
        QString target = QDir(dest).filePath(info.fileName());
        if (info.isDir()) copyDirRec(src, target);
        else QFile::copy(src, target);
    }
    // Wenn zuvor ausgeschnitten, Originale löschen
    if (cutOperationActive) {
        for (const QString &src : cutPaths) {
            QFileInfo info(src);
            if (info.isDir()) QDir(src).removeRecursively();
            else QFile::remove(src);
        }
        cutPaths.clear();
        cutOperationActive = false;
        QApplication::clipboard()->clear();
    }
    view->update();
}

void cutItems(QListView* view, QFileSystemModel* model, QStringList &cutPaths, bool &cutOperationActive) {
    QModelIndexList sel = view->selectionModel()->selectedIndexes();
    cutPaths.clear();
    for (auto &idx : sel) {
        if (idx.column() != 0) continue;
        cutPaths.append(model->filePath(idx));
    }
    if (cutPaths.isEmpty()) return;
    QMimeData *md = new QMimeData;
    QList<QUrl> urls;
    for (const QString &p : cutPaths) urls.append(QUrl::fromLocalFile(p));
    md->setUrls(urls);
    QApplication::clipboard()->setMimeData(md);
    cutOperationActive = true;
}

void deleteItems(QListView* view, QFileSystemModel* model) {
    QModelIndexList sel = view->selectionModel()->selectedIndexes();
    QStringList paths;
    for (auto &idx : sel) {
        if (idx.column() != 0) continue;
        paths << model->filePath(idx);
    }
    if (paths.isEmpty()) return;
    int reply = QMessageBox::question(view,
        QObject::tr("Löschen bestätigen"),
        QObject::tr("Möchten Sie %n Einträge wirklich in den Papierkorb verschieben?", "", paths.size()),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;
    for (const QString &p : paths) {
        QFileInfo info(p);
        auto moveToTrash = [&](const QString &path) {
#ifdef Q_OS_WIN
            SHFILEOPSTRUCT op = {0};
            op.wFunc = FO_DELETE;
            std::wstring from = QDir::toNativeSeparators(path).toStdWString();
            from.push_back(0);
            op.pFrom = from.c_str();
            op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
            SHFileOperation(&op);
#else
            QDir(path).removeRecursively();
#endif
        };
        moveToTrash(p);
    }
    view->update();
}

void showProperties(QWidget* parent, QListView* view, QFileSystemModel* model) {
    QModelIndexList sel = view->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;
    QString p = model->filePath(sel.first());
    QFileInfo info(p);
    // Dateigröße in Bytes und MB
    qint64 sizeBytes = info.size();
    double sizeMB = sizeBytes/1024.0/1024.0;
    QString sizeMBStr = QString::number(sizeMB, 'f', 2);
    // Medieninfos
    QString durationStr;
    QString resolutionStr;
    QString ext = info.suffix().toLower();
    const QStringList mediaExt = {"avi","mp4","mov","mkv","mp3","wav"};
    if (mediaExt.contains(ext)) {
        QMediaPlayer player;
        player.setSource(QUrl::fromLocalFile(p));
        QEventLoop loop;
        QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, [&loop](auto status){
            if (status==QMediaPlayer::LoadedMedia||status==QMediaPlayer::InvalidMedia) loop.quit();
        });
        player.pause(); loop.exec();
        qint64 dur = player.duration();
        durationStr = QTime::fromMSecsSinceStartOfDay(dur).toString("hh:mm:ss");
        // Auflösung aus MetaData ermitteln
        auto md = player.metaData();
        QVariant resVar = md.value(QMediaMetaData::Resolution);
        if (resVar.isValid()) {
            QSize res = resVar.toSize();
            resolutionStr = QString("%1 x %2").arg(res.width()).arg(res.height());
        }
    }
    // Details zusammenstellen
    QString details = QObject::tr("Pfad: %1\nGröße: %2 Bytes (%3 MB)\nZuletzt geändert: %4").arg(
        info.absoluteFilePath(),
        QString::number(sizeBytes),
        sizeMBStr,
        info.lastModified().toString());
    if (!durationStr.isEmpty()) details += QObject::tr("\nDauer: %1").arg(durationStr);
    if (!resolutionStr.isEmpty()) details += QObject::tr("\nAuflösung: %1").arg(resolutionStr);
    QMessageBox::information(parent, QObject::tr("Eigenschaften"), details);
}

} // namespace FileOperations
