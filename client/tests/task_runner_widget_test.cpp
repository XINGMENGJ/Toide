#include <QtTest/QtTest>

#include "task_runner/task_runner_widget.h"

#include <QComboBox>
#include <QTemporaryDir>

class TaskRunnerWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void loadTasksFromWorkspacePopulatesTaskList();
};

void TaskRunnerWidgetTest::loadTasksFromWorkspacePopulatesTaskList()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
    tasksFile.write(R"({
  "tasks": [
    {
      "name": "Build",
      "command": "cmake --build build",
      "workingDirectory": "${workspaceRoot}"
    },
    {
      "name": "Test",
      "command": "ctest --test-dir build --output-on-failure",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})");
    tasksFile.close();

    TaskRunnerWidget widget;

    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *taskSelector = widget.findChild<QComboBox *>(QStringLiteral("taskSelector"));
    QVERIFY(taskSelector != nullptr);
    QCOMPARE(taskSelector->count(), 2);
    QCOMPARE(taskSelector->itemText(0), QStringLiteral("Build"));
    QCOMPARE(taskSelector->itemText(1), QStringLiteral("Test"));
}

QTEST_MAIN(TaskRunnerWidgetTest)

#include "task_runner_widget_test.moc"
