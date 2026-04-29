#include "app/main_window.h"

#include <QAction>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Toide"));
    resize(1280, 800);

    createActions();
    createLayout();

    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::createActions()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->setObjectName(QStringLiteral("fileMenu"));

    openProjectAction_ = fileMenu->addAction(QStringLiteral("&Open Project"));
    openProjectAction_->setObjectName(QStringLiteral("openProjectAction"));

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
    auto *fileExplorer = new QTreeView(this);
    fileExplorer->setObjectName(QStringLiteral("fileExplorerPanel"));
    fileExplorer->setHeaderHidden(true);

    auto *editorTabs = new QTabWidget(this);
    editorTabs->setObjectName(QStringLiteral("editorTabs"));
    editorTabs->setTabsClosable(true);
    editorTabs->addTab(new QTextEdit(editorTabs), QStringLiteral("Welcome"));

    auto *collaborationPanel = new QListWidget(this);
    collaborationPanel->setObjectName(QStringLiteral("collaborationPanel"));
    collaborationPanel->addItem(QStringLiteral("Collaboration events will appear here."));

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setObjectName(QStringLiteral("mainSplitter"));
    mainSplitter->addWidget(fileExplorer);
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
