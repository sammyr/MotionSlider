#include "about.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QApplication>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Über Bildblick");
    setMinimumWidth(400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QString version = QApplication::applicationVersion();
    QLabel* title = new QLabel("<b>Bildblick</b> – Ihr Medienbetrachter", this);
    title->setAlignment(Qt::AlignCenter);
    QLabel* versionLabel = new QLabel("Version: " + (version.isEmpty() ? "1.0" : version), this);
    versionLabel->setAlignment(Qt::AlignCenter);
    QLabel* copyright = new QLabel("© 2025 Sammy Richter", this);
    copyright->setAlignment(Qt::AlignCenter);
    QLabel* info = new QLabel("Multimedia-Explorer und Bild-/Videobetrachter\nEntwickelt mit Qt 6", this);
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(versionLabel);
    layout->addWidget(copyright);
    layout->addWidget(info);
    QDialogButtonBox* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(btnBox);
}
