#include "task_runner/task_execution_request.h"

#include <QDir>

TaskExecutionRequest TaskExecutionRequest::fromTask(const TaskDefinition &task, const QString &workspaceRoot)
{
    auto workingDirectory = task.workingDirectory.trimmed();
    if (workingDirectory.isEmpty()) {
        workingDirectory = workspaceRoot;
    }

    workingDirectory.replace(QStringLiteral("${workspaceRoot}"), workspaceRoot);

    return TaskExecutionRequest{
        .name = task.name,
        .command = task.command,
        .workingDirectory = QDir::cleanPath(workingDirectory),
    };
}
