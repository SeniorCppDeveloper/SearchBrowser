TARGET = QSearch
VERSION = 1.0.0

TEMPLATE = app

QT += core widgets webenginewidgets network

CONFIG += c++17

SOURCES += \
    SearchBrowser.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

CONFIG(debug, debug|release) {
    DEFINES += QT_DEBUG
}

unix:!macx {
    QMAKE_CXXFLAGS += -Wall -Wextra
}
macx {
    ICON = resources/icons/app.icns
}
win32 {

}

QMAKE_TARGET_PRODUCT = QSearch
QMAKE_TARGET_DESCRIPTION = "Быстрая поисковая система"
QMAKE_TARGET_COPYRIGHT = "Copyright © 2026"
