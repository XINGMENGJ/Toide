#include <QtTest/QtTest>

#include "workspace/workspace_manager.h"

#include <QSignalSpy>
#include <QTemporaryDir>

class WorkspaceManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void openProjectAcceptsExistingDirectory();
    void openProjectRejectsMissingDirectory();
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

QTEST_MAIN(WorkspaceManagerTest)

#include "workspace_manager_test.moc"
