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
    ThumbnailDelegate(int maxLength, QObject* parent = nullptr);
    ThumbnailDelegate(QObject* parent = nullptr);
    int getMaxLength() const { return m_maxLength; }
    
    // Paint-Methode überschreiben, um Thumbnails anzuzeigen
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    
    // Status der Thumbnail-Generierung setzen
    void setGeneratingThumbnails(bool generating);

private:
    bool m_generatingThumbnails; // Flag, ob gerade Thumbnails generiert werden
    int m_maxLength = 10;
};
