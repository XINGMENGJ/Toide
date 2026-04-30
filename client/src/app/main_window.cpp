#include "app/main_window.h"

#include "collaboration/collaboration_panel_widget.h"
#include "editor/editor_area_widget.h"
#include "file_explorer/file_explorer_widget.h"
#include "git/git_status_widget.h"
#include "network/network_client.h"
#include "task_runner/task_runner_widget.h"
#include "workspace/recent_project_store.h"
#include "workspace/workspace_manager.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QStandardPaths>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QKeySequence>
#include <QProcess>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , recentProjectStore_(std::make_unique<RecentProjectStore>(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                                               + QStringLiteral("/recent-projects.ini")))
    , networkClient_(new NetworkClient(this))
    , workspaceManager_(new WorkspaceManager(this))
{
    setWindowTitle(QStringLiteral("Toide"));
    resize(1280, 800);

    createActions();
    createLayout();

    statusBar()->showMessage(QStringLiteral("Ready"));

    connect(workspaceManager_, &WorkspaceManager::projectOpened, this, &MainWindow::openProjectDirectory);
    openDefaultWorkspace();
}

MainWindow::~MainWindow() = default;

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->setObjectName(QStringLiteral("fileMenu"));

    openProjectAction_ = fileMenu->addAction(QStringLiteral("&Open Project"));
    openProjectAction_->setObjectName(QStringLiteral("openProjectAction"));
    connect(openProjectAction_, &QAction::triggered, this, &MainWindow::chooseProjectDirectory);

    saveAction_ = fileMenu->addAction(QStringLiteral("&Save"));
    saveAction_->setObjectName(QStringLiteral("saveAction"));
    saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::saveCurrentFile);

    fileMenu->addSeparator();

    exitAction_ = fileMenu->addAction(QStringLiteral("E&xit"));
    exitAction_->setObjectName(QStringLiteral("exitAction"));
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    auto *mainToolBar = addToolBar(QStringLiteral("Main"));
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    mainToolBar->addAction(openProjectAction_);
    mainToolBar->addAction(saveAction_);
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
    outputTabs->addTab(taskRunner_, QStringLiteral("Tasks"));
    outputTabs->addTab(gitStatus_, QStringLiteral("Git"));

    auto *rootSplitter = new QSplitter(Qt::Vertical, this);
    rootSplitter->setObjectName(QStringLiteral("rootSplitter"));
    rootSplitter->addWidget(mainSplitter);
    rootSplitter->addWidget(outputTabs);
    rootSplitter->setStretchFactor(0, 5);
    rootSplitter->setStretchFactor(1, 1);

    setCentralWidget(rootSplitter);

    connect(fileExplorer_, &FileExplorerWidget::fileOpenRequested, editorArea_, &EditorAreaWidget::openFile);
    connect(taskRunner_, &TaskRunnerWidget::diagnosticOpenRequested, editorArea_, &EditorAreaWidget::openFileAt);
    connect(gitStatus_, &GitStatusWidget::openTerminalRequested, this, &MainWindow::openWorkspaceTerminal);
    connect(collaborationPanel_, &CollaborationPanelWidget::serverHealthCheckRequested, this, [this](const QUrl &baseUrl) {
        networkClient_->checkHealth(baseUrl);
    });
    connect(networkClient_, &NetworkClient::healthChecked, collaborationPanel_, &CollaborationPanelWidget::setServerStatus);
}

void MainWindow::chooseProjectDirectory()
{
    const auto selectedDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Open Project"),
        workspaceManager_->currentProjectPath());

    if (!selectedDirectory.isEmpty()) {
        workspaceManager_->openProject(selectedDirectory);
    }
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
    fileExplorer_->setProjectRoot(projectPath);
    taskRunner_->loadTasksFromWorkspace(projectPath);
    gitStatus_->loadStatusFromWorkspace(projectPath);
    collaborationPanel_->setWorkspaceKey(projectPath);
    statusBar()->showMessage(QStringLiteral("Opened project: %1").arg(projectPath));
}

void MainWindow::openWorkspaceTerminal(const QString &projectPath)
{
    if (projectPath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("No workspace is open."), 3000);
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
        QMessageBox::warning(this, QStringLiteral("Open Terminal Failed"), QStringLiteral("Could not open a terminal for this workspace."));
        return;
    }

    statusBar()->showMessage(QStringLiteral("Opened terminal: %1").arg(projectPath), 3000);
}

void MainWindow::saveCurrentFile()
{
    if (editorArea_->saveCurrentFile()) {
        statusBar()->showMessage(QStringLiteral("File saved"), 3000);
        return;
    }

    QMessageBox::warning(this, QStringLiteral("Save Failed"), QStringLiteral("No file is open or the file could not be saved."));
}
