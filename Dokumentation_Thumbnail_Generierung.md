# Dokumentation: Thumbnail-Generierung in DoktorBildPlus

## Überblick

Die Thumbnail-Generierung ist ein zentraler Bestandteil der DoktorBildPlus Anwendung. Sie ermöglicht das Anzeigen von Vorschaubildern für Bild- und Videodateien in der Dateiliste. Diese Dokumentation erklärt detailliert, wie der Prozess funktioniert.

## Architektur und Klassendesign

Der Thumbnail-Generierungsprozess ist auf zwei Hauptklassen aufgeteilt:

1. **ThumbnailGenerator**: Erzeugt die Thumbnail-Dateien für Bilder und Videos
2. **ThumbnailDelegate**: Zeigt die Thumbnails in der QListView-Komponente an

### ThumbnailGenerator

Diese Klasse übernimmt die Erzeugung von Thumbnails für alle Dateien in einem Ordner.

#### Wichtige Methoden

- `generateThumbnailsForFolder(QListView* folderView, QFileSystemModel* folderModel)`: Hauptmethode zur Generierung von Thumbnails für alle Dateien im aktuellen Ordner

#### Funktionsweise

1. **Initialisierung**:
   - Prüft, ob der Thumbnail-Ordner existiert und erstellt ihn bei Bedarf
   - Ermittelt den aktuellen Ordnerpfad aus dem QFileSystemModel

2. **Verarbeitung jeder Datei**:
   - Iteriert durch alle Dateien im Ordner
   - Überspringt Dateien, für die bereits Thumbnails existieren
   - Bestimmt den MIME-Typ jeder Datei zur Unterscheidung von Bildern und Videos

3. **Bild-Thumbnails**:
   - Für Bilddateien wird die Originaldatei direkt geladen
   - Das Bild wird auf die Zielgröße (120x110 Pixel) skaliert, mit Beibehaltung des Seitenverhältnisses
   - Das skalierte Bild wird als PNG-Datei gespeichert

4. **Video-Thumbnails**:
   - Die Methode versucht zuerst, FFmpeg zu nutzen, um einen Frame aus dem Video zu extrahieren
   - Es wird nach einer FFmpeg-Installation im Anwendungsverzeichnis oder im System-PATH gesucht
   - FFmpeg wird mit folgenden Parametern aufgerufen:
     - `-y`: Vorhandene Dateien überschreiben
     - `-ss 00:00:02`: 2 Sekunden in das Video springen
     - `-vframes 1`: Einen einzelnen Frame extrahieren
     - `-q:v 2`: Hohe Qualität für den extrahierten Frame
   - Der Frame wird skaliert und als Thumbnail gespeichert
   - Falls FFmpeg fehlt oder der Prozess fehlschlägt, wird ein generisches Thumbnail erzeugt:
     - Ein farbiger Hintergrund, basierend auf dem Dateinamen (Farbe wird deterministisch berechnet)
     - Ein weißes Play-Symbol 
     - Der Dateiname unten im Bild
     - Die Dateigröße oben rechts im Bild

5. **Speicherung**:
   - Alle Thumbnails werden im Unterordner "thumbnails" des Anwendungsverzeichnisses gespeichert
   - Das Namensschema ist: `[Dateiname]_[Erweiterung]_thumb.png`
   - Beispiel: Für "video.mp4" wird "video_mp4_thumb.png" erstellt

### ThumbnailDelegate

Diese Klasse stellt die Thumbnails in der QListView-Komponente dar.

#### Wichtige Methoden

- `paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)`: Überschreibt die Standard-Darstellungsmethode, um Thumbnails anzuzeigen

#### Funktionsweise

1. **Thumbnail-Suche**:
   - Für jedes Element in der Liste wird geprüft, ob es sich um eine Datei handelt
   - Der Pfad zum potenziellen Thumbnail wird berechnet
   - Prüft, ob das Thumbnail existiert

2. **Darstellung**:
   - Wenn ein Thumbnail existiert, wird es anstelle des Standardsymbols gezeichnet
   - Falls kein Thumbnail gefunden wird, wird auf die Standarddarstellung zurückgegriffen

## Integration in die Anwendung

Die Thumbnail-Generierung wird an folgenden Stellen ausgelöst:

1. Beim Öffnen eines Ordners durch Doppelklick
2. Beim Wechsel des Laufwerks
3. Bei der Auswahl einer Datei
4. Manuell durch den Kontextmenüeintrag "Alle Thumbnails löschen"

## Fehlerbehandlung

- Für Dateien, bei denen die Thumbnail-Generierung fehlschlägt, wird die Standarddarstellung verwendet
- Für Videos, bei denen FFmpeg nicht verfügbar ist, wird ein generisches Thumbnail mit individueller Farbe erzeugt
- Ein Kontextmenüpunkt erlaubt das Löschen und Neugenerieren aller Thumbnails bei Problemen

## Erweiterungsmöglichkeiten

1. **Caching-Strategie**: Thumbnails könnten nach Datum oder Häufigkeit der Nutzung verwaltet werden
2. **Fortschrittsanzeige**: Bei vielen Dateien könnte eine Fortschrittsanzeige implementiert werden
3. **Asynchrone Generierung**: Thumbnails könnten im Hintergrund erzeugt werden, um die Benutzeroberfläche reaktionsschnell zu halten
4. **Konfigurierbare Größe**: Die Thumbnail-Größe könnte vom Benutzer anpassbar gemacht werden
