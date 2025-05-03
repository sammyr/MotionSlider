cmake_minimum_required(VERSION 3.5)
project(DoktorBildPlus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 COMPONENTS Widgets Multimedia MultimediaWidgets REQUIRED)
set(CMAKE_AUTOMOC ON)

if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

add_executable(DoktorBildPlus main.cpp 
    MainWindow.cpp 
    NameShortenDelegate.cpp 
    ThumbnailDelegate.cpp 
    ThumbnailGenerator.cpp 
    SettingsManager.cpp 
    ExternalTools.cpp 
    MediaViewerPanel.cpp
    VideoGraphicsView.cpp
    ContextMenuManager.cpp
    ScrollConfig.cpp
    FileOperations.cpp
    ScrollNavigator.cpp
)

# Windows-Icon-Ressource für Explorer/Taskleiste einbinden
if (WIN32)
    target_sources(DoktorBildPlus PRIVATE appicon.rc)
endif()

target_link_libraries(DoktorBildPlus Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)

# Qt-Resource-Datei für Icons einbinden
qt6_add_resources(GENERATED_RESOURCES resources.qrc)
target_sources(DoktorBildPlus PRIVATE ${GENERATED_RESOURCES})

# Kopiere Einstellungsdateien ins Build-Verzeichnis
configure_file(${CMAKE_SOURCE_DIR}/program-settings.json ${CMAKE_BINARY_DIR}/program-settings.json COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/window-settings.json ${CMAKE_BINARY_DIR}/window-settings.json COPYONLY)
