#include <QtTest/QtTest>

#include "task_runner/task_execution_request.h"

class TaskExecutionRequestTest final : public QObject {
    Q_OBJECT

private slots:
    void fromTaskReplacesWorkspaceRootInWorkingDirectory();
    void fromTaskUsesWorkspaceRootWhenWorkingDirectoryIsEmpty();
};

void TaskExecutionRequestTest::fromTaskReplacesWorkspaceRootInWorkingDirectory()
{
    const TaskDefinition task{
        .name = QStringLiteral("Build"),
        .command = QStringLiteral("cmake --build build"),
        .workingDirectory = QStringLiteral("${workspaceRoot}/build"),
    };

    const auto request = TaskExecutionRequest::fromTask(task, QStringLiteral("E:/Qtcode/Toide"));

    QCOMPARE(request.name, QStringLiteral("Build"));
    QCOMPARE(request.command, QStringLiteral("cmake --build build"));
    QCOMPARE(request.workingDirectory, QStringLiteral("E:/Qtcode/Toide/build"));
}

void TaskExecutionRequestTest::fromTaskUsesWorkspaceRootWhenWorkingDirectoryIsEmpty()
{
    const TaskDefinition task{
        .name = QStringLiteral("Test"),
        .command = QStringLiteral("ctest --output-on-failure"),
        .workingDirectory = {},
    };

    const auto request = TaskExecutionRequest::fromTask(task, QStringLiteral("E:/Qtcode/Toide"));

    QCOMPARE(request.workingDirectory, QStringLiteral("E:/Qtcode/Toide"));
}

QTEST_MAIN(TaskExecutionRequestTest)

#include "task_execution_request_test.moc"
