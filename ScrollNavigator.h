#pragma once

#include <QObject>
#include <QListView>
#include <QFileSystemModel>

class ScrollNavigator : public QObject {
    Q_OBJECT
public:
    ScrollNavigator(QListView* view, QFileSystemModel* model);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QListView* m_view;
    QFileSystemModel* m_model;
};
