#pragma once

#include <QObject>
#include <QProcess>

struct TaskExecutionRequest;

class TaskProcessRunner final : public QObject {
    Q_OBJECT

public:
    explicit TaskProcessRunner(QObject *parent = nullptr);

    [[nodiscard]] bool isRunning() const;
    bool start(const TaskExecutionRequest &request);
    void stop();

signals:
    void outputReceived(const QString &output);
    void errorReceived(const QString &error);
    void finished(int exitCode);

private:
    QProcess process_;
};
