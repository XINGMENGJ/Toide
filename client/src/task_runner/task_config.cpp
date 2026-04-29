#include "task_runner/task_config.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

} // namespace

TaskConfigLoadResult::TaskConfigLoadResult(TaskConfig config)
    : hasValue_(true)
    , config_(std::move(config))
{
}

bool TaskConfigLoadResult::has_value() const
{
    return hasValue_;
}

const TaskConfig &TaskConfigLoadResult::value() const
{
    return config_;
}

const TaskConfig *TaskConfigLoadResult::operator->() const
{
    return &config_;
}

const TaskConfig &TaskConfigLoadResult::operator*() const
{
    return config_;
}

TaskConfigLoadResult TaskConfig::loadFromFile(const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, QStringLiteral("Unable to open task config file."));
        return {};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("Invalid tasks.json: %1").arg(parseError.errorString()));
        return {};
    }

    const auto root = document.object();
    const auto tasksValue = root.value(QStringLiteral("tasks"));
    if (!tasksValue.isArray()) {
        setError(errorMessage, QStringLiteral("tasks.json must contain a tasks array."));
        return {};
    }

    TaskConfig config;
    const auto tasks = tasksValue.toArray();
    for (int index = 0; index < tasks.size(); ++index) {
        const auto taskValue = tasks.at(index);
        if (!taskValue.isObject()) {
            setError(errorMessage, QStringLiteral("Task %1 must be an object.").arg(index));
            return {};
        }

        const auto taskObject = taskValue.toObject();
        const auto name = taskObject.value(QStringLiteral("name")).toString().trimmed();
        const auto command = taskObject.value(QStringLiteral("command")).toString().trimmed();
        const auto workingDirectory = taskObject.value(QStringLiteral("workingDirectory")).toString().trimmed();

        if (name.isEmpty()) {
            setError(errorMessage, QStringLiteral("Task %1 is missing name.").arg(index));
            return {};
        }

        if (command.isEmpty()) {
            setError(errorMessage, QStringLiteral("Task '%1' is missing command.").arg(name));
            return {};
        }

        TaskDefinition task;
        task.name = name;
        task.command = command;
        task.workingDirectory = workingDirectory;
        config.tasks.push_back(task);
    }

    return TaskConfigLoadResult(config);
}
