#include "ScrollNavigator.h"
#include "ScrollConfig.h"
#include <QWheelEvent>
#include <QDebug>
#include <QModelIndex>

ScrollNavigator::ScrollNavigator(QListView* view, QFileSystemModel* model)
    : QObject(view), m_view(view), m_model(model) {
    m_view->viewport()->installEventFilter(this);
}

bool ScrollNavigator::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_view->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* we = static_cast<QWheelEvent*>(event);
        int y = static_cast<int>(we->position().y());
        int h = m_view->viewport()->height();
        qDebug() << "[DEBUG] WheelEvent y=" << y << " h=" << h;
        int navPercent = ScrollConfig::navigationAreaPercent();
        int threshold = static_cast<int>(h * (100 - navPercent) / 100.0);
        if (y < threshold) return false;
        qDebug() << "[DEBUG] In navigation region";
        QModelIndex idx = m_view->currentIndex();
        if (idx.isValid()) {
            int count = m_model->rowCount(idx.parent());
            int row = idx.row();
            int delta = (we->angleDelta().y() > 0 ? -1 : 1);
            int newRow = qBound(0, row + delta, count - 1);
            QModelIndex newIdx = m_model->index(newRow, idx.column(), idx.parent());
            m_view->setCurrentIndex(newIdx);
            m_view->scrollTo(newIdx);
            return true;
        }
        return false;
    }
    return QObject::eventFilter(watched, event);
}
