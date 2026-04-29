#include "task_runner/task_runner_widget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

TaskRunnerWidget::TaskRunnerWidget(QWidget *parent)
    : QWidget(parent)
    , taskSelector_(new QComboBox(this))
    , runButton_(new QPushButton(QStringLiteral("Run"), this))
    , outputView_(new QTextEdit(this))
{
    taskSelector_->setObjectName(QStringLiteral("taskSelector"));
    runButton_->setObjectName(QStringLiteral("runTaskButton"));
    outputView_->setObjectName(QStringLiteral("taskOutputView"));
    outputView_->setReadOnly(true);

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->addWidget(new QLabel(QStringLiteral("Task:"), this));
    toolbarLayout->addWidget(taskSelector_, 1);
    toolbarLayout->addWidget(runButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(toolbarLayout);
    layout->addWidget(outputView_, 1);
}

bool TaskRunnerWidget::loadTasksFromWorkspace(const QString &workspaceRoot)
{
    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(workspaceRoot + QStringLiteral("/.toide/tasks.json"), &errorMessage);
    if (!config.has_value()) {
        taskSelector_->clear();
        outputView_->setPlainText(errorMessage);
        return false;
    }

    taskConfig_ = *config;
    taskSelector_->clear();
    for (const auto &task : taskConfig_.tasks) {
        taskSelector_->addItem(task.name);
    }

    outputView_->clear();
    return true;
}
