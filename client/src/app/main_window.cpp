#include "app/main_window.h"

#include "file_explorer/file_explorer_widget.h"
#include "workspace/workspace_manager.h"

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , workspaceManager_(new WorkspaceManager(this))
{
    setWindowTitle(QStringLiteral("Toide"));
    resize(1280, 800);

    createActions();
    createLayout();

    statusBar()->showMessage(QStringLiteral("Ready"));

    connect(workspaceManager_, &WorkspaceManager::projectOpened, this, &MainWindow::openProjectDirectory);
}

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->setObjectName(QStringLiteral("fileMenu"));

    openProjectAction_ = fileMenu->addAction(QStringLiteral("&Open Project"));
    openProjectAction_->setObjectName(QStringLiteral("openProjectAction"));
    connect(openProjectAction_, &QAction::triggered, this, &MainWindow::chooseProjectDirectory);

    fileMenu->addSeparator();

    exitAction_ = fileMenu->addAction(QStringLiteral("E&xit"));
    exitAction_->setObjectName(QStringLiteral("exitAction"));
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);

    auto *mainToolBar = addToolBar(QStringLiteral("Main"));
    mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    mainToolBar->addAction(openProjectAction_);
}

void MainWindow::createLayout()
{
    fileExplorer_ = new FileExplorerWidget(this);

    auto *editorTabs = new QTabWidget(this);
    editorTabs->setObjectName(QStringLiteral("editorTabs"));
    editorTabs->setTabsClosable(true);
    editorTabs->addTab(new QTextEdit(editorTabs), QStringLiteral("Welcome"));

    auto *collaborationPanel = new QListWidget(this);
    collaborationPanel->setObjectName(QStringLiteral("collaborationPanel"));
    collaborationPanel->addItem(QStringLiteral("Collaboration events will appear here."));

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setObjectName(QStringLiteral("mainSplitter"));
    mainSplitter->addWidget(fileExplorer_);
    mainSplitter->addWidget(editorTabs);
    mainSplitter->addWidget(collaborationPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);
    mainSplitter->setStretchFactor(2, 1);

    auto *outputTabs = new QTabWidget(this);
    outputTabs->setObjectName(QStringLiteral("outputTabs"));
    outputTabs->addTab(new QLabel(QStringLiteral("Task output will appear here."), outputTabs), QStringLiteral("Tasks"));
    outputTabs->addTab(new QLabel(QStringLiteral("Git status will appear here."), outputTabs), QStringLiteral("Git"));

    auto *rootSplitter = new QSplitter(Qt::Vertical, this);
    rootSplitter->setObjectName(QStringLiteral("rootSplitter"));
    rootSplitter->addWidget(mainSplitter);
    rootSplitter->addWidget(outputTabs);
    rootSplitter->setStretchFactor(0, 5);
    rootSplitter->setStretchFactor(1, 1);

    setCentralWidget(rootSplitter);
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

void MainWindow::openProjectDirectory(const QString &projectPath)
{
    fileExplorer_->setProjectRoot(projectPath);
    statusBar()->showMessage(QStringLiteral("Opened project: %1").arg(projectPath));
}
