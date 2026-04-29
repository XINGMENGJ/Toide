#include <QtTest/QtTest>

#include "task_runner/task_runner_widget.h"

#include <QComboBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextEdit>

class TaskRunnerWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void loadTasksFromWorkspacePopulatesTaskList();
    void clickingRunExecutesSelectedTaskAndDisplaysOutput();
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

void TaskRunnerWidgetTest::clickingRunExecutesSelectedTaskAndDisplaysOutput()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
    tasksFile.write(R"({
  "tasks": [
    {
      "name": "Echo",
      "command": "echo WidgetTask",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})");
    tasksFile.close();

    TaskRunnerWidget widget;
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *outputView = widget.findChild<QTextEdit *>(QStringLiteral("taskOutputView"));
    QVERIFY(runButton != nullptr);
    QVERIFY(outputView != nullptr);

    QTest::mouseClick(runButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(outputView->toPlainText().contains(QStringLiteral("WidgetTask")), 3000);
}

QTEST_MAIN(TaskRunnerWidgetTest)

#include "task_runner_widget_test.moc"
