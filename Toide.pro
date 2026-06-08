# Fallback when qmake MSVC kit runs without cl/vcvars (QMAKE_MSC_VER empty). Adjust 1939 to your _MSC_VER if needed.
win32-msvc* {
    isEmpty(QMAKE_MSC_VER) {
        QMAKE_MSC_VER = 1939
    }
}

QT += core gui network widgets

qtHaveModule(websockets) {
    message("Qt WebSockets: enabled for Toide collaboration client")
    QT += websockets
    SOURCES += client/src/network/collaboration_websocket_client.cpp
    HEADERS += client/src/network/collaboration_websocket_client.h
    DEFINES += TOIDE_HAVE_QT_WEBSOCKETS
} else:equals(TOIDE_FORCE_QT_WEBSOCKETS, 1) {
    warning("TOIDE_FORCE_QT_WEBSOCKETS=1: skipping qtHaveModule(websockets); link errors mean the module is not installed for this Qt")
    message("Qt prefix: $$[QT_INSTALL_PREFIX]")
    QT += websockets
    SOURCES += client/src/network/collaboration_websocket_client.cpp
    HEADERS += client/src/network/collaboration_websocket_client.h
    DEFINES += TOIDE_HAVE_QT_WEBSOCKETS
} else {
    warning("Qt WebSockets module not found; collaboration channel UI will be disabled")
    message("Hint: install \"Qt WebSockets\" for this Qt via Qt Maintenance Tool, then run qmake again (Qt in use: $$[QT_INSTALL_PREFIX])")
    message("Or retry qmake with: TOIDE_FORCE_QT_WEBSOCKETS=1 (only if the library is already installed but detection failed)")
}

CONFIG += c++20
CONFIG -= app_bundle

TEMPLATE = app
TARGET = Toide

# Hide extra console when launching the GUI on Windows (CMake uses qt_add_executable WIN32 for the same).
win32:CONFIG += windows

win32:RC_ICONS = client/resources/app.ico

INCLUDEPATH += client/src

DEFINES += TOIDE_SOURCE_DIR=\\\"$$PWD\\\"

SOURCES += \
    client/src/main.cpp \
    client/src/app/main_window.cpp \
    client/src/collaboration/collaboration_panel_widget.cpp \
    client/src/editor/editor_area_widget.cpp \
    client/src/editor/editor_tab.cpp \
    client/src/file_explorer/file_explorer_widget.cpp \
    client/src/git/git_status_widget.cpp \
    client/src/network/network_client.cpp \
    client/src/settings/server_endpoint_settings.cpp \
    client/src/task_runner/task_config.cpp \
    client/src/task_runner/task_diagnostic_parser.cpp \
    client/src/task_runner/task_execution_request.cpp \
    client/src/task_runner/task_process_runner.cpp \
    client/src/task_runner/task_runner_widget.cpp \
    client/src/workspace/recent_project_store.cpp \
    client/src/workspace/workspace_compile_widget.cpp \
    client/src/workspace/workspace_manager.cpp \
    client/src/workspace/workspace_meta.cpp

HEADERS += \
    client/src/app/main_window.h \
    client/src/collaboration/collaboration_panel_widget.h \
    client/src/editor/editor_area_widget.h \
    client/src/editor/editor_tab.h \
    client/src/file_explorer/file_explorer_widget.h \
    client/src/git/git_status_widget.h \
    client/src/network/network_client.h \
    client/src/settings/server_endpoint_settings.h \
    client/src/task_runner/task_config.h \
    client/src/task_runner/task_diagnostic_parser.h \
    client/src/task_runner/task_execution_request.h \
    client/src/task_runner/task_process_runner.h \
    client/src/task_runner/task_runner_widget.h \
    client/src/workspace/recent_project_store.h \
    client/src/workspace/workspace_compile_widget.h \
    client/src/workspace/workspace_manager.h \
    client/src/workspace/workspace_meta.h

RESOURCES += client/resources/toide_changelog.qrc

DESTDIR = bin
OBJECTS_DIR = build/qmake/obj
MOC_DIR = build/qmake/moc
RCC_DIR = build/qmake/rcc
UI_DIR = build/qmake/ui

win32-msvc:QMAKE_CXXFLAGS += /std:c++20 /Zc:__cplusplus /utf-8 /W4 /permissive-
win32-g++:QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8
