#include "task_runner/task_process_runner.h"

#include "task_runner/task_execution_request.h"

#include <QStringList>

TaskProcessRunner::TaskProcessRunner(QObject *parent)
    : QObject(parent)
{
    process_.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&process_, &QProcess::readyReadStandardOutput, this, [this]() {
        emit outputReceived(QString::fromLocal8Bit(process_.readAllStandardOutput()));
    });

    connect(&process_, &QProcess::readyReadStandardError, this, [this]() {
        emit errorReceived(QString::fromLocal8Bit(process_.readAllStandardError()));
    });

    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus) {
        emit finished(exitCode);
    });
}

bool TaskProcessRunner::isRunning() const
{
    return process_.state() != QProcess::NotRunning;
}

bool TaskProcessRunner::start(const TaskExecutionRequest &request)
{
    if (isRunning() || request.command.trimmed().isEmpty()) {
        return false;
    }

    process_.setWorkingDirectory(request.workingDirectory);
#ifdef Q_OS_WIN
    process_.start(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/C"), request.command});
#else
    process_.start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), request.command});
#endif
    return process_.waitForStarted(1000);
}

void TaskProcessRunner::stop()
{
    if (!isRunning()) {
        return;
    }

    process_.terminate();
}
