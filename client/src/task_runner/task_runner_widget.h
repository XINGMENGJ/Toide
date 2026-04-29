#pragma once

#include "task_runner/task_config.h"

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
    QComboBox *taskSelector_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QTextEdit *outputView_ = nullptr;
    TaskConfig taskConfig_;
};
