QT += widgets

CONFIG += c++20
CONFIG -= app_bundle

TEMPLATE = app
TARGET = Toide

INCLUDEPATH += client/src

SOURCES += \
    client/src/main.cpp \
    client/src/app/main_window.cpp \
    client/src/editor/editor_area_widget.cpp \
    client/src/editor/editor_tab.cpp \
    client/src/file_explorer/file_explorer_widget.cpp \
    client/src/task_runner/task_config.cpp \
    client/src/task_runner/task_execution_request.cpp \
    client/src/task_runner/task_process_runner.cpp \
    client/src/workspace/recent_project_store.cpp \
    client/src/workspace/workspace_manager.cpp

HEADERS += \
    client/src/app/main_window.h \
    client/src/editor/editor_area_widget.h \
    client/src/editor/editor_tab.h \
    client/src/file_explorer/file_explorer_widget.h \
    client/src/task_runner/task_config.h \
    client/src/task_runner/task_execution_request.h \
    client/src/task_runner/task_process_runner.h \
    client/src/workspace/recent_project_store.h \
    client/src/workspace/workspace_manager.h

DESTDIR = bin
OBJECTS_DIR = build/qmake/obj
MOC_DIR = build/qmake/moc
RCC_DIR = build/qmake/rcc
UI_DIR = build/qmake/ui

win32-msvc:QMAKE_CXXFLAGS += /W4 /permissive-
win32-g++:QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
