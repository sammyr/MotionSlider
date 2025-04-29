#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QCoreApplication>
#include <QFile>
#include <QIcon>

class ThumbnailDelegate : public QStyledItemDelegate {
public:
    ThumbnailDelegate(QObject* parent = nullptr);
    
    // Paint-Methode überschreiben, um Thumbnails anzuzeigen
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    
    // Status der Thumbnail-Generierung setzen
    void setGeneratingThumbnails(bool generating);

private:
    bool m_generatingThumbnails; // Flag, ob gerade Thumbnails generiert werden
};
