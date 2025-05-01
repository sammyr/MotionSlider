#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QListView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHelpEvent>
#include <QToolTip>
#include <QDebug>

class NameShortenDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    NameShortenDelegate(int maxLen, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), maxLength(maxLen) {}

    QString elidedText(const QString& text) const {
        if (text.length() > maxLength) {
            // qDebug() << "[NameShortenDelegate] Kürze: " << text << "→" << text.left(maxLength) + QChar(0x2026);
            return text.left(maxLength) + QChar(0x2026); // …
        }
        // qDebug() << "[NameShortenDelegate] Zeige voll: " << text;
        return text;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text = elidedText(opt.text);

        painter->save();
        // Icon zeichnen
        QRect iconRect = opt.rect;
        iconRect.setHeight(opt.decorationSize.height());
        if (!opt.icon.isNull())
            opt.icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);

        // Text zeichnen (immer gekürzt!)
        QRect textRect = opt.rect;
        textRect.setTop(iconRect.bottom() + 2);
        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, opt.text);
        painter->restore();
    }

protected:
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override {
        QString fullText = index.data().toString();
        QString shownText = elidedText(fullText);
        if (shownText != fullText) {
            QToolTip::showText(event->globalPos(), fullText, view);
        } else {
            QToolTip::hideText();
        }
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
public:
    int getMaxLength() const { return maxLength; }
private:
    int maxLength;
};
