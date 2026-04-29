#pragma once

#include "task_runner/task_config.h"
#include "task_runner/task_process_runner.h"

#include <QWidget>

class QComboBox;
class QPushButton;
class QTextEdit;

class TaskRunnerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit TaskRunnerWidget(QWidget *parent = nullptr);

    bool loadTasksFromWorkspace(const QString &workspaceRoot);

private:
    void runSelectedTask();
    void appendOutput(const QString &output);

    QComboBox *taskSelector_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QTextEdit *outputView_ = nullptr;
    TaskConfig taskConfig_;
    QString workspaceRoot_;
    TaskProcessRunner processRunner_;
};
