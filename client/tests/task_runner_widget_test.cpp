#include <QtTest/QtTest>

#include "task_runner/task_runner_widget.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextEdit>
#include <QUrl>

class TaskRunnerWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void loadTasksFromWorkspacePopulatesTaskList();
    void clickingRunExecutesSelectedTaskAndDisplaysOutput();
    void successfulTaskShowsSucceededStatus();
    void failedTaskShowsExitCodeStatus();
    void compilerOutputShowsDiagnosticSummary();
    void diagnosticLinkRequestsOpeningSourceLocation();
    void stopButtonStopsRunningTaskAndRestoresIdleState();
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

void TaskRunnerWidgetTest::successfulTaskShowsSucceededStatus()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
    tasksFile.write(R"({
  "tasks": [
    {
      "name": "Success",
      "command": "echo Done",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})");
    tasksFile.close();

    TaskRunnerWidget widget;
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *stopButton = widget.findChild<QPushButton *>(QStringLiteral("stopTaskButton"));
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("taskStatusLabel"));
    QVERIFY(runButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(statusLabel != nullptr);

    QTest::mouseClick(runButton, Qt::LeftButton);

    QTRY_COMPARE_WITH_TIMEOUT(statusLabel->text(), QStringLiteral("成功"), 3000);
    QVERIFY(runButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
}

void TaskRunnerWidgetTest::failedTaskShowsExitCodeStatus()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
#ifdef Q_OS_WIN
    const auto command = QStringLiteral("exit /b 7");
#else
    const auto command = QStringLiteral("exit 7");
#endif
    tasksFile.write(QStringLiteral(R"({
  "tasks": [
    {
      "name": "Failure",
      "command": "%1",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})").arg(command).toUtf8());
    tasksFile.close();

    TaskRunnerWidget widget;
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *stopButton = widget.findChild<QPushButton *>(QStringLiteral("stopTaskButton"));
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("taskStatusLabel"));
    QVERIFY(runButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(statusLabel != nullptr);

    QTest::mouseClick(runButton, Qt::LeftButton);

    QTRY_COMPARE_WITH_TIMEOUT(statusLabel->text(), QStringLiteral("失败：退出码 7"), 3000);
    QVERIFY(runButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
}

void TaskRunnerWidgetTest::compilerOutputShowsDiagnosticSummary()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
#ifdef Q_OS_WIN
    const auto command = QStringLiteral("echo src/hello_toide.cpp:5:18: error: expected semicolon && exit /b 1");
#else
    const auto command = QStringLiteral("printf \"src/hello_toide.cpp:5:18: error: expected semicolon\\n\"; exit 1");
#endif
    tasksFile.write(QStringLiteral(R"({
  "tasks": [
    {
      "name": "Compile Failure",
      "command": "%1",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})").arg(command).toUtf8());
    tasksFile.close();

    TaskRunnerWidget widget;
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *outputView = widget.findChild<QTextEdit *>(QStringLiteral("taskOutputView"));
    QVERIFY(runButton != nullptr);
    QVERIFY(outputView != nullptr);

    QTest::mouseClick(runButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(outputView->toPlainText().contains(QStringLiteral("诊断信息：")), 3000);
    QVERIFY(outputView->toPlainText().contains(QStringLiteral("error: src/hello_toide.cpp:5:18: expected semicolon")));
}

void TaskRunnerWidgetTest::diagnosticLinkRequestsOpeningSourceLocation()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
#ifdef Q_OS_WIN
    const auto command = QStringLiteral("echo src/hello_toide.cpp:5:18: error: expected semicolon && exit /b 1");
#else
    const auto command = QStringLiteral("printf \"src/hello_toide.cpp:5:18: error: expected semicolon\\n\"; exit 1");
#endif
    tasksFile.write(QStringLiteral(R"({
  "tasks": [
    {
      "name": "Compile Failure",
      "command": "%1",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})").arg(command).toUtf8());
    tasksFile.close();

    TaskRunnerWidget widget;
    QSignalSpy diagnosticSpy(&widget, &TaskRunnerWidget::diagnosticOpenRequested);
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *outputView = widget.findChild<QTextBrowser *>(QStringLiteral("taskOutputView"));
    QVERIFY(runButton != nullptr);
    QVERIFY(outputView != nullptr);

    QTest::mouseClick(runButton, Qt::LeftButton);
    QTRY_VERIFY_WITH_TIMEOUT(outputView->toPlainText().contains(QStringLiteral("诊断信息：")), 3000);

    QVERIFY(QMetaObject::invokeMethod(
        outputView,
        "anchorClicked",
        Qt::DirectConnection,
        Q_ARG(QUrl, QUrl(QStringLiteral("toide-diagnostic:0")))));

    QCOMPARE(diagnosticSpy.count(), 1);
    const auto arguments = diagnosticSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QDir(workspace.path()).filePath(QStringLiteral("src/hello_toide.cpp")));
    QCOMPARE(arguments.at(1).toInt(), 5);
    QCOMPARE(arguments.at(2).toInt(), 18);
}

void TaskRunnerWidgetTest::stopButtonStopsRunningTaskAndRestoresIdleState()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(QDir(workspace.path()).mkpath(QStringLiteral(".toide")));

    QFile tasksFile(workspace.filePath(QStringLiteral(".toide/tasks.json")));
    QVERIFY(tasksFile.open(QIODevice::WriteOnly | QIODevice::Text));
#ifdef Q_OS_WIN
    const auto command = QStringLiteral("ping 127.0.0.1 -n 6");
#else
    const auto command = QStringLiteral("sleep 5");
#endif
    tasksFile.write(QStringLiteral(R"({
  "tasks": [
    {
      "name": "Long Running",
      "command": "%1",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})").arg(command).toUtf8());
    tasksFile.close();

    TaskRunnerWidget widget;
    QVERIFY(widget.loadTasksFromWorkspace(workspace.path()));

    auto *runButton = widget.findChild<QPushButton *>(QStringLiteral("runTaskButton"));
    auto *stopButton = widget.findChild<QPushButton *>(QStringLiteral("stopTaskButton"));
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("taskStatusLabel"));
    QVERIFY(runButton != nullptr);
    QVERIFY(stopButton != nullptr);
    QVERIFY(statusLabel != nullptr);
    QVERIFY(runButton->isEnabled());
    QVERIFY(!stopButton->isEnabled());
    QCOMPARE(statusLabel->text(), QStringLiteral("空闲"));

    QTest::mouseClick(runButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(!runButton->isEnabled(), 1000);
    QVERIFY(stopButton->isEnabled());
    QCOMPARE(statusLabel->text(), QStringLiteral("运行中：Long Running"));

    QTest::mouseClick(stopButton, Qt::LeftButton);

    QTRY_VERIFY_WITH_TIMEOUT(runButton->isEnabled(), 3000);
    QVERIFY(!stopButton->isEnabled());
    QCOMPARE(statusLabel->text(), QStringLiteral("空闲"));
}

QTEST_MAIN(TaskRunnerWidgetTest)

#include "task_runner_widget_test.moc"
