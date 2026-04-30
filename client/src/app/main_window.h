#pragma once

#include <QMainWindow>

#include <memory>

class FileExplorerWidget;
class CollaborationPanelWidget;
class EditorAreaWidget;
class GitStatusWidget;
class NetworkClient;
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
    void openWorkspaceTerminal(const QString &projectPath);
    void saveCurrentFile();

    QAction *openProjectAction_ = nullptr;
    QAction *saveAction_ = nullptr;
    QAction *exitAction_ = nullptr;
    std::unique_ptr<RecentProjectStore> recentProjectStore_;
    NetworkClient *networkClient_ = nullptr;
    WorkspaceManager *workspaceManager_ = nullptr;
    FileExplorerWidget *fileExplorer_ = nullptr;
    EditorAreaWidget *editorArea_ = nullptr;
    GitStatusWidget *gitStatus_ = nullptr;
    CollaborationPanelWidget *collaborationPanel_ = nullptr;
    TaskRunnerWidget *taskRunner_ = nullptr;
};
