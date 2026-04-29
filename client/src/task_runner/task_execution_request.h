#pragma once

#include "task_runner/task_config.h"

#include <QString>

struct TaskExecutionRequest final {
    QString name;
    QString command;
    QString workingDirectory;

    static TaskExecutionRequest fromTask(const TaskDefinition &task, const QString &workspaceRoot);
};
