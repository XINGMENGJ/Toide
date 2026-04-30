#pragma once

#include <QMainWindow>

#include <memory>

#include <QString>

class QLabel;

class FileExplorerWidget;
class CollaborationPanelWidget;
class EditorAreaWidget;
class GitStatusWidget;
class NetworkClient;
class RecentProjectStore;
class TaskRunnerWidget;
class WorkspaceCompileWidget;
class WorkspaceManager;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void createActions();
    void createLayout();
    void chooseProjectDirectory();
    void showLoginDialog();
    void openDefaultWorkspace();
    void openProjectDirectory(const QString &projectPath);
    void openWorkspaceTerminal(const QString &projectPath);
    void saveCurrentFile();
    void updateAuthChrome();
    void signOut();
    void onEditorCurrentFileChanged(const QString &absolutePath);
    void refreshServerFileVersion(const QString &absolutePath);
    void pushSavedFileToServer(const QString &absolutePath);
    void showChangelogDialog();

    QAction *openProjectAction_ = nullptr;
    QAction *signInAction_ = nullptr;
    QAction *signOutAction_ = nullptr;
    QAction *saveAction_ = nullptr;
    QAction *compileWorkspaceAction_ = nullptr;
    QAction *runWorkspaceAction_ = nullptr;
    QAction *viewChangelogAction_ = nullptr;
    QAction *exitAction_ = nullptr;
    QLabel *authStatusPermanentLabel_ = nullptr;
    std::unique_ptr<RecentProjectStore> recentProjectStore_;
    NetworkClient *networkClient_ = nullptr;
    WorkspaceManager *workspaceManager_ = nullptr;
    FileExplorerWidget *fileExplorer_ = nullptr;
    EditorAreaWidget *editorArea_ = nullptr;
    GitStatusWidget *gitStatus_ = nullptr;
    WorkspaceCompileWidget *workspaceCompile_ = nullptr;
    CollaborationPanelWidget *collaborationPanel_ = nullptr;
    TaskRunnerWidget *taskRunner_ = nullptr;
    QString currentWorkspaceRoot_;
};
