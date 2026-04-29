#pragma once

#include <QString>
#include <QVector>

struct TaskDefinition final {
    QString name;
    QString command;
    QString workingDirectory;
};

class TaskConfigLoadResult;

struct TaskConfig final {
    QVector<TaskDefinition> tasks;

    static TaskConfigLoadResult loadFromFile(const QString &filePath, QString *errorMessage = nullptr);
};

class TaskConfigLoadResult final {
public:
    TaskConfigLoadResult() = default;
    explicit TaskConfigLoadResult(TaskConfig config);

    [[nodiscard]] bool has_value() const;
    [[nodiscard]] const TaskConfig &value() const;
    [[nodiscard]] const TaskConfig *operator->() const;
    [[nodiscard]] const TaskConfig &operator*() const;

private:
    bool hasValue_ = false;
    TaskConfig config_;
};
