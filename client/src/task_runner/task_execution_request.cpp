#include "task_runner/task_execution_request.h"

#include <QDir>

TaskExecutionRequest TaskExecutionRequest::fromTask(const TaskDefinition &task, const QString &workspaceRoot)
{
    auto workingDirectory = task.workingDirectory.trimmed();
    if (workingDirectory.isEmpty()) {
        workingDirectory = workspaceRoot;
    }

    workingDirectory.replace(QStringLiteral("${workspaceRoot}"), workspaceRoot);

    TaskExecutionRequest request;
    request.name = task.name;
    request.command = task.command;
    request.workingDirectory = QDir::cleanPath(workingDirectory);
    return request;
}
