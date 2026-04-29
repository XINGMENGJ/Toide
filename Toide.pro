QT += widgets

CONFIG += c++20
CONFIG -= app_bundle

TEMPLATE = app
TARGET = Toide

INCLUDEPATH += client/src

SOURCES += \
    client/src/main.cpp \
    client/src/app/main_window.cpp \
    client/src/file_explorer/file_explorer_widget.cpp \
    client/src/workspace/workspace_manager.cpp

HEADERS += \
    client/src/app/main_window.h \
    client/src/file_explorer/file_explorer_widget.h \
    client/src/workspace/workspace_manager.h

DESTDIR = bin
OBJECTS_DIR = build/qmake/obj
MOC_DIR = build/qmake/moc
RCC_DIR = build/qmake/rcc
UI_DIR = build/qmake/ui

win32-msvc:QMAKE_CXXFLAGS += /W4 /permissive-
win32-g++:QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
