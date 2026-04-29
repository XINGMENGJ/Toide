#pragma once

#include <optional>

#include <QString>
#include <QVector>

struct TaskDefinition final {
    QString name;
    QString command;
    QString workingDirectory;
};

struct TaskConfig final {
    QVector<TaskDefinition> tasks;

    static std::optional<TaskConfig> loadFromFile(const QString &filePath, QString *errorMessage = nullptr);
};
