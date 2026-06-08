#pragma once

#include <QMainWindow>

#include <memory>

#include <QString>
#include <QUrl>
#include <QVector>

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
    void applyCollaborationAndChromeForWorkspace(const QString &projectPath);
    void updateWorkspaceChrome();
    void createServerWorkspaceFlow();
    void openServerWorkspaceFlow();
    void continuePullAfterManifest(bool ok, const QString &message, const QJsonArray &files);
    void onPullLatestFile(bool ok,
                          const QString &message,
                          const QString &relativePath,
                          const QString &content,
                          qint64 version);
    void clearPullState();

    QAction *openProjectAction_ = nullptr;
    QAction *newServerWorkspaceAction_ = nullptr;
    QAction *openServerWorkspaceAction_ = nullptr;
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
    QLabel *workspacePathStatusLabel_ = nullptr;
    QString currentWorkspaceRoot_;
    QString currentWorkspaceLabel_;
    QString pullLocalRoot_;
    QString pullProjectKey_;
    QString pullDisplayName_;
    QUrl pullBaseUrl_;
    QString pullToken_;
    QVector<QPair<QString, qint64>> pullQueue_;
    int pullIndex_ = 0;
};
