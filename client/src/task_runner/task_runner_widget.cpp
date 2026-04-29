#include "task_runner/task_runner_widget.h"

#include "task_runner/task_execution_request.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextCursor>
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

    connect(runButton_, &QPushButton::clicked, this, &TaskRunnerWidget::runSelectedTask);
    connect(&processRunner_, &TaskProcessRunner::outputReceived, this, &TaskRunnerWidget::appendOutput);
    connect(&processRunner_, &TaskProcessRunner::errorReceived, this, &TaskRunnerWidget::appendOutput);
    connect(&processRunner_, &TaskProcessRunner::finished, this, [this](int exitCode) {
        appendOutput(QStringLiteral("\nProcess finished with exit code %1\n").arg(exitCode));
        runButton_->setEnabled(true);
    });
}

bool TaskRunnerWidget::loadTasksFromWorkspace(const QString &workspaceRoot)
{
    workspaceRoot_ = workspaceRoot;
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

void TaskRunnerWidget::runSelectedTask()
{
    const auto taskIndex = taskSelector_->currentIndex();
    if (taskIndex < 0 || taskIndex >= taskConfig_.tasks.size()) {
        outputView_->setPlainText(QStringLiteral("No task selected."));
        return;
    }

    outputView_->clear();
    const auto request = TaskExecutionRequest::fromTask(taskConfig_.tasks.at(taskIndex), workspaceRoot_);
    runButton_->setEnabled(false);
    if (!processRunner_.start(request)) {
        runButton_->setEnabled(true);
        outputView_->setPlainText(QStringLiteral("Failed to start task."));
    }
}

void TaskRunnerWidget::appendOutput(const QString &output)
{
    outputView_->moveCursor(QTextCursor::End);
    outputView_->insertPlainText(output);
    outputView_->moveCursor(QTextCursor::End);
}
