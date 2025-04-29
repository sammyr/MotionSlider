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
)
target_link_libraries(DoktorBildPlus Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)
