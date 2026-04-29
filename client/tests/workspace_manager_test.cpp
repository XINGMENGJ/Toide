#include <QtTest/QtTest>

#include "workspace/workspace_manager.h"

#include <QSignalSpy>
#include <QTemporaryDir>

class WorkspaceManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void openProjectAcceptsExistingDirectory();
    void openProjectRejectsMissingDirectory();
    void findDefaultExampleWorkspaceSearchesUpFromRoot();
    void findDefaultExampleWorkspaceFindsConfiguredSourceTree();
};

void WorkspaceManagerTest::openProjectAcceptsExistingDirectory()
{
    QTemporaryDir projectDir;
    QVERIFY(projectDir.isValid());

    WorkspaceManager manager;
    QSignalSpy openedSpy(&manager, &WorkspaceManager::projectOpened);

    QVERIFY(manager.openProject(projectDir.path()));

    QCOMPARE(manager.currentProjectPath(), projectDir.path());
    QCOMPARE(manager.currentProjectName(), QFileInfo(projectDir.path()).fileName());
    QCOMPARE(openedSpy.count(), 1);
    QCOMPARE(openedSpy.takeFirst().at(0).toString(), projectDir.path());
}

void WorkspaceManagerTest::openProjectRejectsMissingDirectory()
{
    WorkspaceManager manager;
    QSignalSpy openedSpy(&manager, &WorkspaceManager::projectOpened);

    QVERIFY(!manager.openProject(QStringLiteral("Z:/toide/path/that/does/not/exist")));

    QVERIFY(manager.currentProjectPath().isEmpty());
    QVERIFY(manager.currentProjectName().isEmpty());
    QCOMPARE(openedSpy.count(), 0);
}

void WorkspaceManagerTest::findDefaultExampleWorkspaceSearchesUpFromRoot()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QDir root(directory.path());
    QVERIFY(root.mkpath(QStringLiteral("examples/default-workspace")));
    QVERIFY(root.mkpath(QStringLiteral("build/client")));

    const auto searchRoot = root.absoluteFilePath(QStringLiteral("build/client"));
    const auto expected = QDir::cleanPath(root.absoluteFilePath(QStringLiteral("examples/default-workspace")));

    QCOMPARE(WorkspaceManager::findDefaultExampleWorkspace(QStringList{searchRoot}), expected);
}

void WorkspaceManagerTest::findDefaultExampleWorkspaceFindsConfiguredSourceTree()
{
#ifdef TOIDE_SOURCE_DIR
    const auto workspace = WorkspaceManager::findDefaultExampleWorkspace(QStringList{QStringLiteral(TOIDE_SOURCE_DIR)});

    QVERIFY(!workspace.isEmpty());
    QVERIFY(workspace.endsWith(QStringLiteral("examples/default-workspace")));
#else
    QFAIL("TOIDE_SOURCE_DIR must be defined by the build system.");
#endif
}

QTEST_MAIN(WorkspaceManagerTest)

#include "workspace_manager_test.moc"
