#include "app/main_window.h"

#include "collaboration/collaboration_panel_widget.h"
#include "editor/editor_area_widget.h"
#include "file_explorer/file_explorer_widget.h"
#include "git/git_status_widget.h"
#include "network/network_client.h"
#include "task_runner/task_runner_widget.h"
#include "workspace/recent_project_store.h"
#include "workspace/workspace_compile_widget.h"
#include "workspace/workspace_manager.h"
#include "settings/server_endpoint_settings.h"

#include <QAction>
#include <QDialog>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QTextBrowser>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QKeySequence>
#include <QPointer>
#include <QProcess>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QUrl>

namespace {

QString relativePathInWorkspace(const QString &workspaceRoot, const QString &absolutePath)
{
    if (absolutePath.isEmpty()) {
        return {};
    }
    if (workspaceRoot.isEmpty()) {
        return QFileInfo(absolutePath).fileName();
    }
    const QString rel = QDir(workspaceRoot).relativeFilePath(absolutePath);
    if (rel.startsWith(QStringLiteral(".."))) {
        return {};
    }
    return QDir::fromNativeSeparators(rel);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , recentProjectStore_(std::make_unique<RecentProjectStore>(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                                               + QStringLiteral("/recent-projects.ini")))
    , networkClient_(new NetworkClient(this))
    , workspaceManager_(new WorkspaceManager(this))
{
    setWindowTitle(QStringLiteral("Toide 协作 IDE"));
    resize(1280, 800);

    createActions();
    createLayout();

    collaborationPanel_->clearAuthSession();

    statusBar()->showMessage(QStringLiteral("就绪"));

    connect(workspaceManager_, &WorkspaceManager::projectOpened, this, &MainWindow::openProjectDirectory);
    openDefaultWorkspace();
}

MainWindow::~MainWindow() = default;

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->setObjectName(QStringLiteral("fileMenu"));

    openProjectAction_ = fileMenu->addAction(QStringLiteral("打开项目(&O)..."));
    openProjectAction_->setObjectName(QStringLiteral("openProjectAction"));
    connect(openProjectAction_, &QAction::triggered, this, &MainWindow::chooseProjectDirectory);

    fileMenu->addSeparator();

    signInAction_ = fileMenu->addAction(QStringLiteral("登录/注册服务器(&L)..."));
    signInAction_->setObjectName(QStringLiteral("signInAction"));
    signInAction_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+L")));
    signInAction_->setStatusTip(QStringLiteral("注册或登录以使用协作功能"));
    connect(signInAction_, &QAction::triggered, this, &MainWindow::showLoginDialog);

    saveAction_ = fileMenu->addAction(QStringLiteral("保存(&S)"));
    saveAction_->setObjectName(QStringLiteral("saveAction"));
    saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::saveCurrentFile);

    fileMenu->addSeparator();

    exitAction_ = fileMenu->addAction(QStringLiteral("退出(&X)"));
    exitAction_->setObjectName(QStringLiteral("exitAction"));
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    auto *accountMenu = menuBar()->addMenu(QStringLiteral("账号(&A)"));
    accountMenu->setObjectName(QStringLiteral("accountMenu"));
    accountMenu->addAction(signInAction_);
    signOutAction_ = accountMenu->addAction(QStringLiteral("退出登录(&O)"));
    signOutAction_->setObjectName(QStringLiteral("signOutAction"));
    signOutAction_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+K")));
    connect(signOutAction_, &QAction::triggered, this, &MainWindow::signOut);

    auto *mainToolBar = addToolBar(QStringLiteral("主工具栏"));
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    mainToolBar->addAction(openProjectAction_);
    mainToolBar->addAction(signInAction_);
    mainToolBar->addAction(saveAction_);

    compileWorkspaceAction_ = mainToolBar->addAction(QStringLiteral("编译 include/src"));
    compileWorkspaceAction_->setObjectName(QStringLiteral("compileWorkspaceAction"));
    compileWorkspaceAction_->setStatusTip(QStringLiteral("检查工作区 include 与 src 下的 C/C++ 源文件与头文件"));

    runWorkspaceAction_ = mainToolBar->addAction(QStringLiteral("运行工作区"));
    runWorkspaceAction_->setObjectName(QStringLiteral("runWorkspaceAction"));
    runWorkspaceAction_->setStatusTip(QStringLiteral("链接 include 与 src 下的源文件并运行（输出在编译页）"));

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->setObjectName(QStringLiteral("helpMenu"));
    viewChangelogAction_ = helpMenu->addAction(QStringLiteral("更新日志(&C)…"));
    viewChangelogAction_->setObjectName(QStringLiteral("viewChangelogAction"));
    connect(viewChangelogAction_, &QAction::triggered, this, &MainWindow::showChangelogDialog);
}

void MainWindow::createLayout()
{
    fileExplorer_ = new FileExplorerWidget(this);

    editorArea_ = new EditorAreaWidget(this);

    collaborationPanel_ = new CollaborationPanelWidget(this);
    collaborationPanel_->setObjectName(QStringLiteral("collaborationPanel"));

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setObjectName(QStringLiteral("mainSplitter"));
    mainSplitter->addWidget(fileExplorer_);
    mainSplitter->addWidget(editorArea_);
    mainSplitter->addWidget(collaborationPanel_);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);
    mainSplitter->setStretchFactor(2, 1);

    auto *outputTabs = new QTabWidget(this);
    outputTabs->setObjectName(QStringLiteral("outputTabs"));
    taskRunner_ = new TaskRunnerWidget(outputTabs);
    gitStatus_ = new GitStatusWidget(outputTabs);
    workspaceCompile_ = new WorkspaceCompileWidget(outputTabs);
    outputTabs->addTab(taskRunner_, QStringLiteral("任务"));
    outputTabs->addTab(gitStatus_, QStringLiteral("Git"));
    outputTabs->addTab(workspaceCompile_, QStringLiteral("编译"));

    auto *rootSplitter = new QSplitter(Qt::Vertical, this);
    rootSplitter->setObjectName(QStringLiteral("rootSplitter"));
    rootSplitter->addWidget(mainSplitter);
    rootSplitter->addWidget(outputTabs);
    rootSplitter->setStretchFactor(0, 5);
    rootSplitter->setStretchFactor(1, 1);

    authStatusPermanentLabel_ = new QLabel(this);
    authStatusPermanentLabel_->setObjectName(QStringLiteral("authStatusPermanentLabel"));
    statusBar()->addPermanentWidget(authStatusPermanentLabel_);

    setCentralWidget(rootSplitter);

    connect(fileExplorer_, &FileExplorerWidget::fileOpenRequested, editorArea_, &EditorAreaWidget::openFile);
    connect(taskRunner_, &TaskRunnerWidget::diagnosticOpenRequested, editorArea_, &EditorAreaWidget::openFileAt);
    connect(gitStatus_, &GitStatusWidget::openTerminalRequested, this, &MainWindow::openWorkspaceTerminal);
    connect(collaborationPanel_, &CollaborationPanelWidget::serverHealthCheckRequested, this, [this](const QUrl &baseUrl) {
        networkClient_->checkHealth(baseUrl);
    });
    connect(networkClient_, &NetworkClient::healthChecked, collaborationPanel_, &CollaborationPanelWidget::setServerStatus);
    connect(networkClient_, &NetworkClient::authFinished, this, [this](bool ok, const QString &message, const QString &token, const QString &username) {
        if (!ok) {
            statusBar()->showMessage(QStringLiteral("登录失败：%1").arg(message), 6000);
            return;
        }
        collaborationPanel_->setAuthSession(token, username);
        statusBar()->showMessage(QStringLiteral("已登录：%1").arg(username), 5000);
    });
    connect(editorArea_, &EditorAreaWidget::currentFilePathChanged, collaborationPanel_, &CollaborationPanelWidget::notifyCurrentFile);
    connect(editorArea_, &EditorAreaWidget::currentFileSaved, collaborationPanel_, &CollaborationPanelWidget::notifyLocalFileSaved);
    connect(editorArea_, &EditorAreaWidget::fileTextEdited, collaborationPanel_, &CollaborationPanelWidget::notifyLocalTextEdited);
    connect(editorArea_, &EditorAreaWidget::cursorPositionChanged, collaborationPanel_, &CollaborationPanelWidget::notifyLocalCursorMoved);
    connect(collaborationPanel_, &CollaborationPanelWidget::remoteFileUpdated, editorArea_, &EditorAreaWidget::applyRemoteFileText);
    connect(collaborationPanel_, &CollaborationPanelWidget::remoteCursorMoved, editorArea_, &EditorAreaWidget::showRemoteCursor);
    connect(collaborationPanel_, &CollaborationPanelWidget::collaborationRosterSynced, this, [this]() {
        collaborationPanel_->notifyCurrentFile(editorArea_->currentFilePath());
    });
    connect(collaborationPanel_, &CollaborationPanelWidget::loginRequested, this, &MainWindow::showLoginDialog);
    connect(collaborationPanel_, &CollaborationPanelWidget::authSessionChanged, this, &MainWindow::updateAuthChrome);

    connect(editorArea_, &EditorAreaWidget::currentFilePathChanged, this, &MainWindow::onEditorCurrentFileChanged);
    connect(networkClient_, &NetworkClient::workspaceFileVersionFetched, this,
            [this](bool ok, const QString &message, const QString &absoluteFilePath, qint64 version) {
                if (ok && version >= 0) {
                    editorArea_->setServerDocVersionForPath(absoluteFilePath, version);
                } else if (!message.isEmpty() && !absoluteFilePath.isEmpty()) {
                    Q_UNUSED(message);
                }
            });
    connect(networkClient_, &NetworkClient::workspaceFileUploadFinished, this,
            [this](bool ok, const QString &message, const QString &absoluteFilePath, qint64 newVersion, bool conflict,
                   qint64 serverLatestVersion) {
                if (ok && newVersion >= 0) {
                    editorArea_->setServerDocVersionForPath(absoluteFilePath, newVersion);
                    collaborationPanel_->notifyLocalFileSynced(absoluteFilePath, newVersion);
                    statusBar()->showMessage(QStringLiteral("已同步到服务器，版本 %1").arg(newVersion), 4000);
                    return;
                }
                if (conflict) {
                    QMessageBox::warning(this,
                                         QStringLiteral("版本冲突"),
                                         QStringLiteral("服务器上该文件已被更新（最新版本 %1）。\n请与他人协调后重新保存同步。\n%2")
                                             .arg(serverLatestVersion)
                                             .arg(message));
                    refreshServerFileVersion(absoluteFilePath);
                    return;
                }
                if (!message.isEmpty()) {
                    statusBar()->showMessage(QStringLiteral("同步服务器失败：%1").arg(message), 6000);
                }
            });

    updateAuthChrome();

    connect(compileWorkspaceAction_, &QAction::triggered, this, [this, outputTabs]() {
        if (workspaceCompile_ == nullptr) {
            return;
        }
        outputTabs->setCurrentIndex(outputTabs->indexOf(workspaceCompile_));
        workspaceCompile_->runCompile();
    });
    connect(runWorkspaceAction_, &QAction::triggered, this, [this, outputTabs]() {
        if (workspaceCompile_ == nullptr) {
            return;
        }
        outputTabs->setCurrentIndex(outputTabs->indexOf(workspaceCompile_));
        workspaceCompile_->runWorkspaceProgram();
    });
}

void MainWindow::chooseProjectDirectory()
{
    const auto selectedDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("打开项目"),
        workspaceManager_->currentProjectPath());

    if (!selectedDirectory.isEmpty()) {
        workspaceManager_->openProject(selectedDirectory);
    }
}

void MainWindow::showLoginDialog()
{
    ServerEndpointSettings settings;
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("登录 Toide 协作"));
    dialog.setMinimumWidth(420);

    auto *urlEdit = new QLineEdit(settings.serverBaseUrl(), &dialog);
    auto *userEdit = new QLineEdit(settings.username(), &dialog);
    auto *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);

    auto *errorLabel = new QLabel(&dialog);
    errorLabel->setWordWrap(true);
    errorLabel->setMinimumHeight(22);

    auto *form = new QFormLayout();
    form->addRow(QStringLiteral("服务器地址："), urlEdit);
    form->addRow(QStringLiteral("用户名："), userEdit);
    form->addRow(QStringLiteral("密码："), passwordEdit);

    auto *buttons = new QHBoxLayout;
    auto *loginButton = new QPushButton(QStringLiteral("登录"), &dialog);
    auto *registerButton = new QPushButton(QStringLiteral("注册"), &dialog);
    buttons->addStretch(1);
    buttons->addWidget(registerButton);
    buttons->addWidget(loginButton);

    auto *outer = new QVBoxLayout(&dialog);
    outer->addLayout(form);
    outer->addWidget(errorLabel);
    outer->addLayout(buttons);

    QPointer<QDialog> dialogPtr(&dialog);
    QMetaObject::Connection authConn;

    const auto finishPendingAuth = [dialogPtr, errorLabel, loginButton, registerButton, &authConn](bool ok,
                                                                                                   const QString &message) {
        QObject::disconnect(authConn);
        loginButton->setEnabled(true);
        registerButton->setEnabled(true);
        if (!dialogPtr) {
            return;
        }
        if (ok) {
            dialogPtr->accept();
            return;
        }
        errorLabel->setText(message);
    };

    const auto submit = [&](bool registerUser) {
        errorLabel->clear();
        QObject::disconnect(authConn);
        ServerEndpointSettings s;
        s.setServerBaseUrl(urlEdit->text());
        const QUrl server(s.serverBaseUrl());
        if (server.scheme().isEmpty() || server.host().isEmpty()) {
            errorLabel->setText(QStringLiteral("请输入有效的服务器地址（例如 http://127.0.0.1:8848）。"));
            return;
        }
        if (userEdit->text().trimmed().isEmpty() || passwordEdit->text().isEmpty()) {
            errorLabel->setText(QStringLiteral("请填写用户名和密码。"));
            return;
        }
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
        authConn = QObject::connect(networkClient_, &NetworkClient::authFinished, &dialog,
                                    [finishPendingAuth](bool ok, const QString &message, const QString &token,
                                                        const QString &username) {
                                        Q_UNUSED(token);
                                        Q_UNUSED(username);
                                        finishPendingAuth(ok, message);
                                    });
        if (registerUser) {
            networkClient_->registerUser(server, userEdit->text(), passwordEdit->text());
        } else {
            networkClient_->login(server, userEdit->text(), passwordEdit->text());
        }
    };

    connect(loginButton, &QPushButton::clicked, &dialog, [submit]() {
        submit(false);
    });
    connect(registerButton, &QPushButton::clicked, &dialog, [submit]() {
        submit(true);
    });

    dialog.exec();
}

void MainWindow::openDefaultWorkspace()
{
    QStringList searchRoots{
        QDir::currentPath(),
        QCoreApplication::applicationDirPath(),
    };

#ifdef TOIDE_SOURCE_DIR
    searchRoots.append(QStringLiteral(TOIDE_SOURCE_DIR));
#endif

    const auto defaultWorkspace = WorkspaceManager::findDefaultExampleWorkspace(searchRoots);

    if (!defaultWorkspace.isEmpty()) {
        workspaceManager_->openProject(defaultWorkspace);
    }
}

void MainWindow::openProjectDirectory(const QString &projectPath)
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    recentProjectStore_->addProject(projectPath);
    currentWorkspaceRoot_ = QFileInfo(projectPath).absoluteFilePath();
    fileExplorer_->setProjectRoot(projectPath);
    taskRunner_->loadTasksFromWorkspace(projectPath);
    gitStatus_->loadStatusFromWorkspace(projectPath);
    if (workspaceCompile_ != nullptr) {
        workspaceCompile_->setWorkspaceRoot(projectPath);
    }
    collaborationPanel_->setWorkspaceKey(projectPath);
    collaborationPanel_->setWorkspaceRoot(projectPath);
    collaborationPanel_->notifyCurrentFile(editorArea_->currentFilePath());
    refreshServerFileVersion(editorArea_->currentFilePath());
    statusBar()->showMessage(QStringLiteral("已打开项目：%1").arg(projectPath));
}

void MainWindow::openWorkspaceTerminal(const QString &projectPath)
{
    if (projectPath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("未打开工作区。"), 3000);
        return;
    }

#if defined(Q_OS_WIN)
    const bool started = QProcess::startDetached(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/K")}, projectPath);
#elif defined(Q_OS_MACOS)
    const bool started = QProcess::startDetached(QStringLiteral("open"), QStringList{QStringLiteral("-a"), QStringLiteral("Terminal"), projectPath});
#else
    const bool started = QProcess::startDetached(QStringLiteral("x-terminal-emulator"), QStringList{}, projectPath);
#endif

    if (!started) {
        QMessageBox::warning(this, QStringLiteral("打开终端失败"), QStringLiteral("无法为此工作区启动终端。"));
        return;
    }

    statusBar()->showMessage(QStringLiteral("已打开终端：%1").arg(projectPath), 3000);
}

void MainWindow::saveCurrentFile()
{
    if (!editorArea_->saveCurrentFile()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("没有打开的文件，或无法写入磁盘。"));
        return;
    }

    statusBar()->showMessage(QStringLiteral("文件已保存到本地，正在同步服务器…"), 4000);
    pushSavedFileToServer(editorArea_->currentFilePath());
}

void MainWindow::updateAuthChrome()
{
    if (collaborationPanel_ == nullptr || authStatusPermanentLabel_ == nullptr) {
        return;
    }
    authStatusPermanentLabel_->setText(collaborationPanel_->authStatusLine());
    if (signOutAction_ != nullptr) {
        signOutAction_->setEnabled(collaborationPanel_->isSignedIn());
    }
}

void MainWindow::signOut()
{
    if (collaborationPanel_ == nullptr || !collaborationPanel_->isSignedIn()) {
        return;
    }
    collaborationPanel_->clearAuthSession();
    statusBar()->showMessage(QStringLiteral("已退出登录"), 4000);
}

void MainWindow::onEditorCurrentFileChanged(const QString &absolutePath)
{
    refreshServerFileVersion(absolutePath);
}

void MainWindow::refreshServerFileVersion(const QString &absolutePath)
{
    if (collaborationPanel_ == nullptr || absolutePath.isEmpty() || currentWorkspaceRoot_.isEmpty()) {
        return;
    }
    if (!collaborationPanel_->isSignedIn()) {
        return;
    }
    const QString token = collaborationPanel_->authBearerToken();
    if (token.isEmpty()) {
        return;
    }
    const QUrl baseUrl(collaborationPanel_->collaborationServerBaseUrl());
    if (baseUrl.scheme().isEmpty() || baseUrl.host().isEmpty()) {
        return;
    }
    const QString rel = relativePathInWorkspace(currentWorkspaceRoot_, absolutePath);
    if (rel.isEmpty()) {
        return;
    }
    networkClient_->fetchWorkspaceFileVersion(baseUrl, collaborationPanel_->collaborationProjectKey(), rel,
                                                absolutePath, token);
}

void MainWindow::showChangelogDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Toide 更新日志"));
    dialog.resize(560, 420);

    auto *browser = new QTextBrowser(&dialog);
    browser->setOpenExternalLinks(false);
    QFile bundled(QStringLiteral(":/toide/app_changelog.txt"));
    if (bundled.open(QIODevice::ReadOnly)) {
        browser->setPlainText(QString::fromUtf8(bundled.readAll()));
    } else {
        browser->setPlainText(QStringLiteral("未找到内置更新日志资源。"));
    }

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(browser);
    dialog.exec();
}

void MainWindow::pushSavedFileToServer(const QString &absolutePath)
{
    if (collaborationPanel_ == nullptr || absolutePath.isEmpty() || currentWorkspaceRoot_.isEmpty()) {
        return;
    }
    if (!collaborationPanel_->isSignedIn()) {
        return;
    }
    const QString token = collaborationPanel_->authBearerToken();
    if (token.isEmpty()) {
        return;
    }
    const QUrl baseUrl(collaborationPanel_->collaborationServerBaseUrl());
    if (baseUrl.scheme().isEmpty() || baseUrl.host().isEmpty()) {
        return;
    }
    const QString rel = relativePathInWorkspace(currentWorkspaceRoot_, absolutePath);
    if (rel.isEmpty()) {
        return;
    }

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        statusBar()->showMessage(QStringLiteral("无法读取文件，跳过同步到服务器"), 5000);
        return;
    }
    const QByteArray raw = file.readAll();
    file.close();
    const QString content = QString::fromUtf8(raw);

    qint64 baseVer = editorArea_->serverDocVersionForPath(absolutePath);
    if (baseVer < 0) {
        baseVer = 0;
    }

    networkClient_->putWorkspaceFileContent(baseUrl, collaborationPanel_->collaborationProjectKey(), rel,
                                            absolutePath, baseVer, content, token);
}
