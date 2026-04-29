#pragma once

#include <QMainWindow>

class FileExplorerWidget;
class EditorAreaWidget;
class WorkspaceManager;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createActions();
    void createLayout();
    void chooseProjectDirectory();
    void openProjectDirectory(const QString &projectPath);
    void saveCurrentFile();

    QAction *openProjectAction_ = nullptr;
    QAction *saveAction_ = nullptr;
    QAction *exitAction_ = nullptr;
    WorkspaceManager *workspaceManager_ = nullptr;
    FileExplorerWidget *fileExplorer_ = nullptr;
    EditorAreaWidget *editorArea_ = nullptr;
};
