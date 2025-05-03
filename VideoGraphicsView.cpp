#include "VideoGraphicsView.h"
#include <QWheelEvent>
#include <cmath>
#include <QCursor>

VideoGraphicsView::VideoGraphicsView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
{
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void VideoGraphicsView::wheelEvent(QWheelEvent* event) {
    int steps = event->angleDelta().y() / 120;
    if (steps != 0) {
        double factor = std::pow(1.1, steps);
        scale(factor, factor);
    }
    event->accept();
}

void VideoGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        isPanning = true;
        lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void VideoGraphicsView::mouseMoveEvent(QMouseEvent* event) {
    if (isPanning && (event->buttons() & Qt::MiddleButton)) {
        QPoint delta = event->pos() - lastPanPoint;
        lastPanPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void VideoGraphicsView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && isPanning) {
        isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}
