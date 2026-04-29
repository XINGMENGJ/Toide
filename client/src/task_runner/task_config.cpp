#include "task_runner/task_config.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

} // namespace

std::optional<TaskConfig> TaskConfig::loadFromFile(const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, QStringLiteral("Unable to open task config file."));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("Invalid tasks.json: %1").arg(parseError.errorString()));
        return std::nullopt;
    }

    const auto root = document.object();
    const auto tasksValue = root.value(QStringLiteral("tasks"));
    if (!tasksValue.isArray()) {
        setError(errorMessage, QStringLiteral("tasks.json must contain a tasks array."));
        return std::nullopt;
    }

    TaskConfig config;
    const auto tasks = tasksValue.toArray();
    for (int index = 0; index < tasks.size(); ++index) {
        const auto taskValue = tasks.at(index);
        if (!taskValue.isObject()) {
            setError(errorMessage, QStringLiteral("Task %1 must be an object.").arg(index));
            return std::nullopt;
        }

        const auto taskObject = taskValue.toObject();
        const auto name = taskObject.value(QStringLiteral("name")).toString().trimmed();
        const auto command = taskObject.value(QStringLiteral("command")).toString().trimmed();
        const auto workingDirectory = taskObject.value(QStringLiteral("workingDirectory")).toString().trimmed();

        if (name.isEmpty()) {
            setError(errorMessage, QStringLiteral("Task %1 is missing name.").arg(index));
            return std::nullopt;
        }

        if (command.isEmpty()) {
            setError(errorMessage, QStringLiteral("Task '%1' is missing command.").arg(name));
            return std::nullopt;
        }

        config.tasks.push_back(TaskDefinition{
            .name = name,
            .command = command,
            .workingDirectory = workingDirectory,
        });
    }

    return config;
}
