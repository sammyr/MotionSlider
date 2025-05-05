#include "ThumbnailDelegate.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QPainter>
#include <QDebug>

ThumbnailDelegate::ThumbnailDelegate(int maxLength, QObject* parent)
    : QStyledItemDelegate(parent), m_generatingThumbnails(false), m_maxLength(maxLength) {}

ThumbnailDelegate::ThumbnailDelegate(QObject* parent)
    : ThumbnailDelegate(10, parent) {}


void ThumbnailDelegate::setGeneratingThumbnails(bool generating) {
    m_generatingThumbnails = generating;
}

void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    static int debugCounter = 0;
    const bool VERBOSE_DEBUG = (debugCounter++ < 10); // Weniger Debug-Ausgaben
    
    QString filePath = index.model()->data(index, QFileSystemModel::FilePathRole).toString();
    QFileInfo info(filePath);
    
    if (!info.isFile() || info.suffix().isEmpty()) {
        // Für Ordner oder Dateien ohne Endung verwenden wir die Standard-Darstellung
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    

    
    // Thumbnail-Pfad berechnen
    QString appDir = QCoreApplication::applicationDirPath() + "/thumbnails/";
    uint pathHash = qHash(info.absoluteFilePath());
    QString thumbPath = appDir + info.completeBaseName() + '_' + QString::number(pathHash) + '_' + info.suffix().toLower() + "_thumb.png";
    
    if (QFile::exists(thumbPath)) {
        // Thumbnail laden
        QPixmap originalThumb(thumbPath);
        
        if (originalThumb.isNull()) {
            qDebug() << "[ThumbnailDelegate] PNG existiert, aber QPixmap konnte NICHT geladen werden:" << thumbPath;
        }
        if (!originalThumb.isNull()) {
            if (VERBOSE_DEBUG) {
                qDebug() << "Thumbnails für" << info.fileName() << "gefunden und geladen";
            }
            
            // NICHT die Standard-Zeichenroutine verwenden, sondern alles selbst zeichnen
            
            // Nur den Selektionsrahmen zeichnen, falls das Item selektiert ist
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, option.palette.highlight());
            }
            
            // Icon-Bereich anpassen und ausblenden
            QStyleOptionViewItem textOption = option;
            textOption.icon = QIcon();
            textOption.decorationSize = QSize(0, 0);
            textOption.features &= ~QStyleOptionViewItem::HasDecoration;
            
            // NICHT nochmal den Dateinamen zeichnen, um Dopplung zu vermeiden
            
            // Jetzt das Thumbnail zeichnen
            QRect iconRect;
            
            // Größe und Position für das Thumbnail berechnen
            int thumbMax = option.decorationSize.width(); // Nutze die vom View vorgegebene Größe
            iconRect.setWidth(thumbMax);
            iconRect.setHeight(thumbMax);
            // Zentrieren im verfügbaren Bereich
            iconRect.moveLeft(option.rect.left() + (option.rect.width() - iconRect.width()) / 2);
            
            // Oben positionieren mit etwas Abstand
            int textHeight = painter->fontMetrics().height() + 4; // Platz für Text + Abstand
            int availableHeight = option.rect.height() - textHeight;
            iconRect.moveTop(option.rect.top() + 5); // 5px von oben
            
            // Thumbnail mit korrektem Seitenverhältnis skalieren und zeichnen
            QPixmap scaledThumb = originalThumb.scaled(iconRect.size(), 
                                                      Qt::KeepAspectRatio, 
                                                      Qt::SmoothTransformation);
            
            // Graue Box ohne Rahmen als Hintergrund für das Thumbnail - minimaler Rand
            QRect bgRect = iconRect;
            bgRect.adjust(-2, -2, 2, 2); // Nur 2px Rand
            painter->fillRect(bgRect, QColor(240, 240, 240, 200)); // Hellgrau mit leichter Transparenz
            
            // Zentrierte Position berechnen für das skalierte Thumbnail
            QRect centeredRect = iconRect;
            if (scaledThumb.width() < iconRect.width()) {
                centeredRect.setLeft(iconRect.left() + (iconRect.width() - scaledThumb.width()) / 2);
                centeredRect.setWidth(scaledThumb.width());
            }
            if (scaledThumb.height() < iconRect.height()) {
                centeredRect.setTop(iconRect.top() + (iconRect.height() - scaledThumb.height()) / 2);
                centeredRect.setHeight(scaledThumb.height());
            }
            
            // Thumbnail zeichnen
            painter->drawPixmap(centeredRect, scaledThumb);
            
            // Dateinamen weiter unten unter der Box anzeigen (20px Abstand statt 5px)
            QRect textRect = option.rect;
            textRect.setTop(iconRect.bottom() + 20); // 20 Pixel unter der Thumbnail-Box
            textRect.setHeight(painter->fontMetrics().height() * 2); // Mehr Platz für Text
            textRect.setLeft(option.rect.left());
            textRect.setRight(option.rect.right());
            
            // Dateinamen zeichnen - auf maximale Länge begrenzen (vom Konstruktor übergeben)
            painter->setPen(Qt::black);
            QFont font = painter->font();
            font.setPointSize(8); // Kleinere Schrift
            painter->setFont(font);
            
            QString displayName = info.fileName();
            int maxLength = m_maxLength;
            if (displayName.length() > maxLength) {
                displayName = displayName.left(maxLength) + "...";
            }
            
            painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, displayName);
            
            return;
        }
    }
    
    // Wenn kein Thumbnail geladen werden konnte, Standard-Darstellung verwenden
    QStyledItemDelegate::paint(painter, option, index);
}
