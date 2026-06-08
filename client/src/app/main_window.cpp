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
#include "workspace/workspace_meta.h"
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
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QListWidgetItem>
#include <QKeySequence>
#include <QPointer>
#include <QProcess>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
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

QString sanitizeLocalDirName(QString s)
{
    const QString forbidden = QStringLiteral("<>:\"/\\|?*");
    QString out;
    out.reserve(s.size());
    for (const QChar c : s) {
        if (forbidden.contains(c)) {
            continue;
        }
        out.append(c);
    }
    out = out.trimmed();
    return out.isEmpty() ? QStringLiteral("workspace") : out;
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
    updateWorkspaceChrome();
}

MainWindow::~MainWindow() = default;

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->setObjectName(QStringLiteral("fileMenu"));

    openProjectAction_ = fileMenu->addAction(QStringLiteral("打开本地项目(&O)..."));
    openProjectAction_->setObjectName(QStringLiteral("openProjectAction"));
    connect(openProjectAction_, &QAction::triggered, this, &MainWindow::chooseProjectDirectory);

    newServerWorkspaceAction_ = fileMenu->addAction(QStringLiteral("新建服务器工作区(&N)…"));
    newServerWorkspaceAction_->setStatusTip(QStringLiteral("登录后在服务器注册工作区并创建本地目录"));
    connect(newServerWorkspaceAction_, &QAction::triggered, this, &MainWindow::createServerWorkspaceFlow);

    openServerWorkspaceAction_ = fileMenu->addAction(QStringLiteral("从服务器打开工作区(&S)…"));
    openServerWorkspaceAction_->setStatusTip(QStringLiteral("浏览服务器上所有已注册工作区并下载到本地"));
    connect(openServerWorkspaceAction_, &QAction::triggered, this, &MainWindow::openServerWorkspaceFlow);

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
    mainToolBar->addAction(newServerWorkspaceAction_);
    mainToolBar->addAction(openServerWorkspaceAction_);
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

    workspacePathStatusLabel_ = new QLabel(this);
    workspacePathStatusLabel_->setObjectName(QStringLiteral("workspacePathStatusLabel"));
    workspacePathStatusLabel_->setMinimumWidth(280);
    statusBar()->addWidget(workspacePathStatusLabel_, 1);

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
                    if (collaborationPanel_ != nullptr && collaborationPanel_->isSignedIn()) {
                        editorArea_->setServerDocVersionForPath(absoluteFilePath, newVersion);
                        collaborationPanel_->notifyLocalFileSynced(absoluteFilePath, newVersion);
                        statusBar()->showMessage(QStringLiteral("已同步到服务器，版本 %1").arg(newVersion), 4000);
                    }
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

    connect(networkClient_, &NetworkClient::workspaceManifestFetched, this, &MainWindow::continuePullAfterManifest);
    connect(networkClient_, &NetworkClient::workspaceLatestFileFetched, this, &MainWindow::onPullLatestFile);

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
    editorArea_->closeAllTabs();
    currentWorkspaceRoot_ = QFileInfo(projectPath).absoluteFilePath();
    fileExplorer_->setProjectRoot(projectPath);
    taskRunner_->loadTasksFromWorkspace(projectPath);
    gitStatus_->loadStatusFromWorkspace(projectPath);
    if (workspaceCompile_ != nullptr) {
        workspaceCompile_->setWorkspaceRoot(projectPath);
    }
    applyCollaborationAndChromeForWorkspace(projectPath);
    collaborationPanel_->setWorkspaceRoot(projectPath);
    collaborationPanel_->notifyCurrentFile(editorArea_->currentFilePath());
    refreshServerFileVersion(editorArea_->currentFilePath());
    statusBar()->showMessage(QStringLiteral("已打开工作区：%1").arg(currentWorkspaceRoot_), 8000);
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

    const QString savedPath = editorArea_->currentFilePath();
    if (fileExplorer_ != nullptr) {
        fileExplorer_->noteFileChangedOnDisk(savedPath);
    }

    if (collaborationPanel_ != nullptr && collaborationPanel_->isSignedIn()) {
        statusBar()->showMessage(QStringLiteral("已保存到本地，随后上传服务器…"), 4000);
        // 下一事件循环再读盘上传：新建文件时避免与刚关闭的写入句柄/索引竞争（Windows 上更稳）。
        QTimer::singleShot(0, this, [this, savedPath]() {
            pushSavedFileToServer(savedPath);
        });
    } else {
        statusBar()->showMessage(QStringLiteral("已保存到本地（未登录，未上传服务器）"), 4000);
    }
}

void MainWindow::updateAuthChrome()
{
    if (collaborationPanel_ == nullptr || authStatusPermanentLabel_ == nullptr) {
        return;
    }
    authStatusPermanentLabel_->setText(collaborationPanel_->authStatusLine());
    const bool in = collaborationPanel_->isSignedIn();
    if (signOutAction_ != nullptr) {
        signOutAction_->setEnabled(in);
    }
    if (newServerWorkspaceAction_ != nullptr) {
        newServerWorkspaceAction_->setEnabled(in);
    }
    if (openServerWorkspaceAction_ != nullptr) {
        openServerWorkspaceAction_->setEnabled(in);
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
    QFile bundled(QStringLiteral(":/toide/resources/app_changelog.txt"));
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

void MainWindow::applyCollaborationAndChromeForWorkspace(const QString &projectPath)
{
    const QString abs = QFileInfo(projectPath).absoluteFilePath();
    if (const auto meta = WorkspaceMeta::load(abs)) {
        collaborationPanel_->setCollaborationProjectKey(meta->serverWorkspaceId);
        currentWorkspaceLabel_ =
            meta->displayName.isEmpty() ? meta->serverWorkspaceId : meta->displayName;
    } else {
        collaborationPanel_->setWorkspaceKey(abs);
        currentWorkspaceLabel_ = QFileInfo(abs).fileName();
    }
    updateWorkspaceChrome();
}

void MainWindow::updateWorkspaceChrome()
{
    if (workspacePathStatusLabel_ != nullptr) {
        workspacePathStatusLabel_->setText(
            QStringLiteral("当前工作区目录：%1")
                .arg(currentWorkspaceRoot_.isEmpty() ? QStringLiteral("(未打开)") : currentWorkspaceRoot_));
    }
    const QString titleName = currentWorkspaceLabel_.isEmpty() ? QStringLiteral("未打开") : currentWorkspaceLabel_;
    setWindowTitle(QStringLiteral("Toide — %1").arg(titleName));
}

void MainWindow::clearPullState()
{
    pullQueue_.clear();
    pullLocalRoot_.clear();
    pullProjectKey_.clear();
    pullDisplayName_.clear();
    pullToken_.clear();
    pullBaseUrl_ = QUrl();
    pullIndex_ = 0;
}

void MainWindow::createServerWorkspaceFlow()
{
    if (collaborationPanel_ == nullptr || !collaborationPanel_->isSignedIn()) {
        QMessageBox::information(this,
                                 QStringLiteral("需要登录"),
                                 QStringLiteral("请先在「账号」中登录服务器，再创建服务器工作区。"));
        return;
    }
    bool nameOk = false;
    const QString name =
        QInputDialog::getText(this, QStringLiteral("新建服务器工作区"), QStringLiteral("工作区显示名称："),
                              QLineEdit::Normal, QString(), &nameOk)
            .trimmed();
    if (!nameOk || name.isEmpty()) {
        return;
    }
    const QString parentDir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择本地父目录（将创建子文件夹）"),
        workspaceManager_->currentProjectPath().isEmpty() ? QDir::homePath()
                                                           : workspaceManager_->currentProjectPath());
    if (parentDir.isEmpty()) {
        return;
    }

    const QUrl baseUrl(collaborationPanel_->collaborationServerBaseUrl());
    const QString token = collaborationPanel_->authBearerToken();

    QMetaObject::Connection conn;
    conn = connect(
        networkClient_, &NetworkClient::workspaceCreated, this,
        [this, conn, parentDir, name](bool ok, const QString &message, const QString &id, const QString &srvName) mutable {
            QObject::disconnect(conn);
            if (!ok) {
                QMessageBox::warning(this, QStringLiteral("创建工作区失败"), message);
                return;
            }
            const QString localSegment =
                sanitizeLocalDirName(srvName) + QLatin1Char('-') + id.left(8).remove(QLatin1Char('-'));
            const QString localRoot = QDir(QDir::cleanPath(parentDir)).filePath(localSegment);
            if (QFileInfo::exists(localRoot)) {
                QMessageBox::warning(this,
                                     QStringLiteral("目录已存在"),
                                     QStringLiteral("目标文件夹已存在：\n%1").arg(localRoot));
                return;
            }
            if (!QDir().mkpath(localRoot)) {
                QMessageBox::warning(this, QStringLiteral("创建失败"), QStringLiteral("无法创建本地目录。"));
                return;
            }
            WorkspaceMeta meta;
            meta.serverWorkspaceId = id;
            meta.displayName = srvName.isEmpty() ? name : srvName;
            if (!WorkspaceMeta::save(localRoot, meta)) {
                QMessageBox::warning(this, QStringLiteral("写入失败"), QStringLiteral("无法写入 .toide/workspace.json。"));
                return;
            }
            const QString toideDir = QDir(localRoot).filePath(QStringLiteral(".toide"));
            if (!QDir().mkpath(toideDir)) {
                QMessageBox::warning(this, QStringLiteral("创建失败"), QStringLiteral("无法创建 .toide 目录。"));
                return;
            }
            QFile tasksFile(QDir(toideDir).filePath(QStringLiteral("tasks.json")));
            if (tasksFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                tasksFile.write("{\n  \"tasks\": []\n}\n");
                tasksFile.close();
            }
            statusBar()->showMessage(QStringLiteral("已在服务器注册工作区并打开本地副本。"), 6000);
            workspaceManager_->openProject(localRoot);
        });
    networkClient_->createWorkspace(baseUrl, name, token);
}

void MainWindow::openServerWorkspaceFlow()
{
    if (collaborationPanel_ == nullptr || !collaborationPanel_->isSignedIn()) {
        QMessageBox::information(this,
                                 QStringLiteral("需要登录"),
                                 QStringLiteral("请先在「账号」中登录服务器，再从服务器打开工作区。"));
        return;
    }
    const QUrl baseUrl(collaborationPanel_->collaborationServerBaseUrl());
    const QString token = collaborationPanel_->authBearerToken();

    QMetaObject::Connection conn;
    conn = connect(networkClient_, &NetworkClient::workspacesListFetched, this,
                   [this, conn, baseUrl, token](bool ok, const QString &message, const QJsonArray &workspaces) mutable {
                       QObject::disconnect(conn);
                       if (!ok) {
                           QMessageBox::warning(this, QStringLiteral("无法获取列表"), message);
                           return;
                       }
                       if (workspaces.isEmpty()) {
                           QMessageBox::information(this,
                                                    QStringLiteral("暂无工作区"),
                                                    QStringLiteral("服务器上还没有任何已注册工作区。"));
                           return;
                       }

                       QDialog pickDialog(this);
                       pickDialog.setWindowTitle(QStringLiteral("选择服务器工作区"));
                       pickDialog.resize(520, 360);
                       auto *listWidget = new QListWidget(&pickDialog);
                       for (const QJsonValue &v : workspaces) {
                           const QJsonObject o = v.toObject();
                           const QString wid = o.value(QStringLiteral("id")).toString();
                           const QString wname = o.value(QStringLiteral("name")).toString();
                           const QString creator =
                               o.value(QStringLiteral("createdByUsername")).toString();
                           auto *item = new QListWidgetItem(
                               QStringLiteral("%1  （创建者：%2）\nID %3")
                                   .arg(wname, creator.isEmpty() ? QStringLiteral("?") : creator, wid));
                           item->setData(Qt::UserRole, wid);
                           item->setData(Qt::UserRole + 1, wname);
                           listWidget->addItem(item);
                       }
                       auto *openBtn = new QPushButton(QStringLiteral("打开并下载"), &pickDialog);
                       auto *cancelBtn = new QPushButton(QStringLiteral("取消"), &pickDialog);
                       auto *row = new QHBoxLayout();
                       row->addStretch(1);
                       row->addWidget(openBtn);
                       row->addWidget(cancelBtn);
                       auto *lay = new QVBoxLayout(&pickDialog);
                       lay->addWidget(listWidget);
                       lay->addLayout(row);
                       QObject::connect(openBtn, &QPushButton::clicked, &pickDialog, &QDialog::accept);
                       QObject::connect(cancelBtn, &QPushButton::clicked, &pickDialog, &QDialog::reject);
                       if (pickDialog.exec() != QDialog::Accepted) {
                           return;
                       }
                       QListWidgetItem *cur = listWidget->currentItem();
                       if (cur == nullptr) {
                           QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请选择一行工作区。"));
                           return;
                       }
                       const QString wsId = cur->data(Qt::UserRole).toString();
                       const QString wsName = cur->data(Qt::UserRole + 1).toString();
                       const QString parentDir = QFileDialog::getExistingDirectory(
                           this,
                           QStringLiteral("选择本地父目录（将创建子文件夹）"),
                           workspaceManager_->currentProjectPath().isEmpty() ? QDir::homePath()
                                                                              : workspaceManager_->currentProjectPath());
                       if (parentDir.isEmpty()) {
                           return;
                       }
                       const QString localSegment =
                           sanitizeLocalDirName(wsName) + QLatin1Char('-') + wsId.left(8).remove(QLatin1Char('-'));
                       const QString localRoot = QDir(QDir::cleanPath(parentDir)).filePath(localSegment);
                       if (QFileInfo::exists(localRoot)) {
                           QMessageBox::warning(
                               this,
                               QStringLiteral("目录已存在"),
                               QStringLiteral("目标文件夹已存在，请删除或更换父目录：\n%1").arg(localRoot));
                           return;
                       }
                       if (!QDir().mkpath(localRoot)) {
                           QMessageBox::warning(this, QStringLiteral("创建失败"), QStringLiteral("无法创建本地目录。"));
                           return;
                       }
                       WorkspaceMeta meta;
                       meta.serverWorkspaceId = wsId;
                       meta.displayName = wsName;
                       if (!WorkspaceMeta::save(localRoot, meta)) {
                           QMessageBox::warning(this, QStringLiteral("写入失败"), QStringLiteral("无法写入 .toide/workspace.json。"));
                           return;
                       }
                       const QString toideDir = QDir(localRoot).filePath(QStringLiteral(".toide"));
                       QFile tasksFile(QDir(toideDir).filePath(QStringLiteral("tasks.json")));
                       if (tasksFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                           tasksFile.write("{\n  \"tasks\": []\n}\n");
                           tasksFile.close();
                       }

                       pullLocalRoot_ = localRoot;
                       pullProjectKey_ = wsId;
                       pullDisplayName_ = wsName;
                       pullToken_ = token;
                       pullBaseUrl_ = baseUrl;
                       pullIndex_ = 0;
                       pullQueue_.clear();
                       statusBar()->showMessage(QStringLiteral("正在从服务器获取文件清单…"), 3000);
                       networkClient_->fetchWorkspaceManifest(baseUrl, wsId, token);
                   });
    networkClient_->listWorkspaces(baseUrl, token);
}

void MainWindow::continuePullAfterManifest(bool ok, const QString &message, const QJsonArray &files)
{
    if (pullLocalRoot_.isEmpty()) {
        return;
    }
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("清单获取失败"), message);
        clearPullState();
        return;
    }
    pullQueue_.clear();
    for (const QJsonValue &v : files) {
        const QJsonObject o = v.toObject();
        const QString path = o.value(QStringLiteral("path")).toString();
        const qint64 ver = o.value(QStringLiteral("version")).toVariant().toLongLong();
        if (!path.isEmpty()) {
            pullQueue_.append(qMakePair(path, ver));
        }
    }
    pullIndex_ = 0;
    if (pullQueue_.isEmpty()) {
        const QString done = pullLocalRoot_;
        clearPullState();
        workspaceManager_->openProject(done);
        statusBar()->showMessage(QStringLiteral("工作区已在本地打开（服务器上尚无文件）。"), 5000);
        return;
    }
    statusBar()->showMessage(QStringLiteral("正在下载文件…"), 3000);
    networkClient_->fetchWorkspaceLatestFile(pullBaseUrl_, pullProjectKey_, pullQueue_.at(0).first, pullToken_);
}

void MainWindow::onPullLatestFile(bool ok,
                                  const QString &message,
                                  const QString &relativePath,
                                  const QString &content,
                                  qint64 version)
{
    if (pullLocalRoot_.isEmpty()) {
        return;
    }
    if (!ok) {
        QMessageBox::warning(this,
                             QStringLiteral("下载文件失败"),
                             QStringLiteral("%1\n%2").arg(relativePath, message));
        clearPullState();
        return;
    }
    const QString fullPath = QDir(pullLocalRoot_).filePath(relativePath);
    const QFileInfo fi(fullPath);
    if (!QDir().mkpath(fi.path())) {
        QMessageBox::warning(this, QStringLiteral("写入失败"), QStringLiteral("无法创建目录：%1").arg(fi.path()));
        clearPullState();
        return;
    }
    QFile out(fullPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("写入失败"), QStringLiteral("无法写入：%1").arg(fullPath));
        clearPullState();
        return;
    }
    out.write(content.toUtf8());
    out.close();
    editorArea_->setServerDocVersionForPath(fullPath, version);

    pullIndex_++;
    if (pullIndex_ >= pullQueue_.size()) {
        const QString done = pullLocalRoot_;
        clearPullState();
        workspaceManager_->openProject(done);
        statusBar()->showMessage(QStringLiteral("已从服务器拉取所有文件并打开工作区。"), 6000);
        return;
    }
    networkClient_->fetchWorkspaceLatestFile(pullBaseUrl_, pullProjectKey_, pullQueue_.at(pullIndex_).first,
                                             pullToken_);
}
