#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPoint>

class VideoGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit VideoGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    bool isPanning = false;
    QPoint lastPanPoint;
};
