#pragma once

#include <QMainWindow>

#include <memory>

class FileExplorerWidget;
class EditorAreaWidget;
class RecentProjectStore;
class TaskRunnerWidget;
class WorkspaceManager;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createActions();
    void createLayout();
    void chooseProjectDirectory();
    void openDefaultWorkspace();
    void openProjectDirectory(const QString &projectPath);
    void saveCurrentFile();

    QAction *openProjectAction_ = nullptr;
    QAction *saveAction_ = nullptr;
    QAction *exitAction_ = nullptr;
    std::unique_ptr<RecentProjectStore> recentProjectStore_;
    WorkspaceManager *workspaceManager_ = nullptr;
    FileExplorerWidget *fileExplorer_ = nullptr;
    EditorAreaWidget *editorArea_ = nullptr;
    TaskRunnerWidget *taskRunner_ = nullptr;
};
