void MainWindow::loadWindowSettings()
{
    QString path = QCoreApplication::applicationDirPath() + "/window-settings.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    // Fensterposition und -größe
    if(obj.contains("window")) {
        QJsonObject win = obj["window"].toObject();
        int x = win.value("x").toInt(100);
        int y = win.value("y").toInt(100);
        int w = win.value("width").toInt(1200);
        int h = win.value("height").toInt(800);
        this->setGeometry(x, y, w, h);
    }
    // Splitter
    if(obj.contains("splitter") && centralWidget()) {
        QSplitter* splitter = qobject_cast<QSplitter*>(centralWidget());
        if(splitter) {
            QJsonArray arr = obj["splitter"].toArray();
            QList<int> sizes;
            for(const QJsonValue& v : arr) sizes << v.toInt();
            splitter->setSizes(sizes);
        }
    }
    // Letztes Verzeichnis
    if(obj.contains("lastDirectory") && folderView && folderModel) {
        QString dir = obj["lastDirectory"].toString();
        QModelIndex idx = folderModel->index(dir);
        if(idx.isValid()) folderView->setRootIndex(idx);
        if(pathEdit) pathEdit->setText(dir);
    }
}

void MainWindow::saveWindowSettings()
{
    QString path = QCoreApplication::applicationDirPath() + "/window-settings.json";
    QJsonObject obj;
    // Fensterposition und -größe
    QRect geom = this->geometry();
    QJsonObject win;
    win["x"] = geom.x(); win["y"] = geom.y();
    win["width"] = geom.width(); win["height"] = geom.height();
    obj["window"] = win;
    // Splitter
    if(centralWidget()) {
        QSplitter* splitter = qobject_cast<QSplitter*>(centralWidget());
        if(splitter) {
            QList<int> sizes = splitter->sizes();
            QJsonArray arr;
            for(int s : sizes) arr.append(s);
            obj["splitter"] = arr;
        }
    }
    // Letztes Verzeichnis
    if(pathEdit) obj["lastDirectory"] = pathEdit->text();
    QJsonDocument doc(obj);
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)) file.write(doc.toJson());
}
