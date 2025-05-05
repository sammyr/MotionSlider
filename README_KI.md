# Bildblick – KI-freundliche Projektdokumentation

## Projektüberblick

**Bildblick** ist eine C++/Qt-Anwendung zum Anzeigen von Bildern und Videos. Sie verwendet Qt6 (Widgets, Multimedia, MultimediaWidgets) und ist für Windows 10 mit MinGW 13.1.0 gebaut. Die Anwendung unterstützt Datei-Öffnen-Dialoge und zeigt Medien direkt im Hauptfenster an.

---

## Projektstruktur (Dateiübersicht)

```
Bildblick/
│
├── build/                    # Build-Ordner, enthält die lauffähige EXE & alle DLLs/Plugins
├── main.cpp                  # Einstiegspunkt, startet QApplication und MainWindow
├── MainWindow.h              # Header für das Hauptfenster (QMainWindow, Q_OBJECT Makro)
├── MainWindow.cpp            # Implementierung des Hauptfensters, Menü, Bild/Video-Logik
├── CMakeLists.txt            # Build-Konfiguration für CMake & Qt6
└── README_KI.md              # Diese Datei
```

---

## Wichtige Dateien und Aufgaben

- **main.cpp**: Startet die QApplication, erzeugt das MainWindow und ruft `.show()` auf.
- **MainWindow.h / MainWindow.cpp**: Definiert das Hauptfenster, Menü, Bild- und Videodarstellung. Nutzt das Q_OBJECT-Makro für Qt-Signale/Slots. Implementiert Datei-Öffnen und Medientyp-Erkennung.
- **CMakeLists.txt**: Legt Projekt, Qt6-Abhängigkeiten, Build-Optionen und Automoc fest. Beispiel:
  ```cmake
  cmake_minimum_required(VERSION 3.5)
  project(Bildblick LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 17)
  find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)
  set(CMAKE_AUTOMOC ON)
  add_executable(Bildblick main.cpp MainWindow.cpp)
  target_link_libraries(Bildblick Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)
  ```

---

## Build- und Deployment-Anleitung (Windows 10, MinGW, Qt6)

1. **Qt & MinGW bereitstellen**
   - Qt 6.9.0 inkl. MinGW 13.1.0 installieren.
   - MinGW-Bin-Verzeichnis zum PATH hinzufügen:
     ```powershell
     $env:PATH += ";C:\Qt\Tools\mingw1310_64\bin"
     ```

2. **Build-Ordner anlegen und Projekt konfigurieren**
   ```powershell
   cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.0/mingw_64"
   ```

3. **Kompilieren**
   ```powershell
   mingw32-make -C build
   ```

4. **Qt-Deployment (DLLs & Plugins kopieren)**
   ```powershell
   & "C:\Qt\6.9.0\mingw_64\bin\windeployqt.exe" "D:\___SYSTEM\Desktop\_NPM\Doktor_Video\Bildblick\build\Bildblick.exe"
   ```
   - Kopiert alle nötigen DLLs und Qt-Plugins (wie qwindows.dll) automatisch in den Build-Ordner.

5. **Starten**
   ```powershell
   .\build\Bildblick.exe
   ```

---

## Hinweise zu DLLs und Qt-Plugins

- Die Anwendung benötigt viele DLLs (Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Multimedia.dll, ...).
- Das Plattform-Plugin `qwindows.dll` muss im Unterordner `platforms` im Build-Verzeichnis liegen (wird von windeployqt erledigt).
- Ohne diese Dateien startet die Anwendung nicht oder bleibt ohne Fehlermeldung geschlossen.

---

## Fehlerbehebung (für KI-Assistenz)

- **vtable-Fehler:**
  - Tritt auf, wenn das Q_OBJECT-Makro fehlt oder CMake/MOC falsch konfiguriert ist.
  - Lösung: Q_OBJECT im Header, CMAKE_AUTOMOC ON, nur .cpp-Dateien in add_executable.
- **Programm startet nicht:**
  - Meist fehlen DLLs oder das Qt-Plattform-Plugin.
  - Lösung: Immer windeployqt nach dem Build ausführen!
- **Keine Fehlermeldung, kein Fenster:**
  - Fast immer ein Anzeichen für fehlende DLLs oder Plugins.

---

## Weiterführend

- Für die Verteilung auf andere Rechner: Den gesamten `build`-Ordner kopieren – keine Qt-Installation auf dem Zielsystem nötig.
- Für neue Features: Weitere Qt-Module in find_package und target_link_libraries ergänzen.
- Für Debugging: EXE aus der Eingabeaufforderung (`cmd.exe`) starten und auf Fehlermeldungen achten.

---

**Diese README_KI.md ist für KI-Assistenten und Entwickler gedacht, die das Projekt automatisiert analysieren, bauen oder erweitern möchten.**
