#pragma once

#include "task_runner/task_config.h"
#include "task_runner/task_process_runner.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QTextEdit;

class TaskRunnerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit TaskRunnerWidget(QWidget *parent = nullptr);

    bool loadTasksFromWorkspace(const QString &workspaceRoot);

private:
    void runSelectedTask();
    void stopRunningTask();
    void appendOutput(const QString &output);
    void setTaskRunning(bool isRunning, const QString &taskName = {});
    void setTaskFinished(int exitCode);

    QComboBox *taskSelector_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QTextEdit *outputView_ = nullptr;
    TaskConfig taskConfig_;
    QString workspaceRoot_;
    bool stopRequested_ = false;
    TaskProcessRunner processRunner_;
};
