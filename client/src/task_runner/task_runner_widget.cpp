#include "task_runner/task_runner_widget.h"

#include "task_runner/task_execution_request.h"

#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

TaskRunnerWidget::TaskRunnerWidget(QWidget *parent)
    : QWidget(parent)
    , taskSelector_(new QComboBox(this))
    , runButton_(new QPushButton(QStringLiteral("运行"), this))
    , stopButton_(new QPushButton(QStringLiteral("停止"), this))
    , statusLabel_(new QLabel(QStringLiteral("空闲"), this))
    , terminalCommandEdit_(new QLineEdit(this))
    , terminalRunButton_(new QPushButton(QStringLiteral("运行命令"), this))
    , outputView_(new QTextBrowser(this))
{
    taskSelector_->setObjectName(QStringLiteral("taskSelector"));
    runButton_->setObjectName(QStringLiteral("runTaskButton"));
    stopButton_->setObjectName(QStringLiteral("stopTaskButton"));
    statusLabel_->setObjectName(QStringLiteral("taskStatusLabel"));
    outputView_->setObjectName(QStringLiteral("taskOutputView"));
    stopButton_->setEnabled(false);
    outputView_->setReadOnly(true);
    outputView_->setOpenLinks(false);
    terminalCommandEdit_->setObjectName(QStringLiteral("terminalCommandLineEdit"));
    terminalRunButton_->setObjectName(QStringLiteral("terminalRunButton"));
    terminalCommandEdit_->setPlaceholderText(QStringLiteral("在工作区中执行的终端命令…"));

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->addWidget(new QLabel(QStringLiteral("任务："), this));
    toolbarLayout->addWidget(taskSelector_, 1);
    toolbarLayout->addWidget(statusLabel_);
    toolbarLayout->addWidget(runButton_);
    toolbarLayout->addWidget(stopButton_);

    auto *terminalLayout = new QHBoxLayout;
    terminalLayout->addWidget(new QLabel(QStringLiteral("终端："), this));
    terminalLayout->addWidget(terminalCommandEdit_, 1);
    terminalLayout->addWidget(terminalRunButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(toolbarLayout);
    layout->addLayout(terminalLayout);
    layout->addWidget(outputView_, 1);

    connect(runButton_, &QPushButton::clicked, this, &TaskRunnerWidget::runSelectedTask);
    connect(stopButton_, &QPushButton::clicked, this, &TaskRunnerWidget::stopRunningTask);
    connect(terminalRunButton_, &QPushButton::clicked, this, [this]() {
        if (!runButton_->isEnabled()) {
            appendOutput(QStringLiteral("\n已有任务在运行，请先停止再执行其他命令。\n"));
            return;
        }
        const QString command = terminalCommandEdit_->text().trimmed();
        if (command.isEmpty()) {
            outputView_->setPlainText(QStringLiteral("未输入终端命令。"));
            return;
        }
        if (!processRunner_.start(TaskExecutionRequest{QStringLiteral("terminal"), command, workspaceRoot_})) {
            outputView_->setPlainText(QStringLiteral("无法启动终端命令。"));
            return;
        }
        outputView_->clear();
        taskOutputBuffer_.clear();
        setTaskRunning(true, QStringLiteral("terminal"));
    });
    connect(outputView_, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        if (url.scheme() != QStringLiteral("toide-diagnostic")) {
            return;
        }

        bool ok = false;
        const auto index = url.toString().section(QLatin1Char(':'), 1).toInt(&ok);
        if (!ok || index < 0 || index >= taskDiagnostics_.size()) {
            return;
        }

        const auto &diagnostic = taskDiagnostics_.at(index);
        emit diagnosticOpenRequested(QDir(workspaceRoot_).filePath(diagnostic.filePath), diagnostic.line, diagnostic.column);
    });
    connect(&processRunner_, &TaskProcessRunner::outputReceived, this, &TaskRunnerWidget::appendOutput);
    connect(&processRunner_, &TaskProcessRunner::errorReceived, this, &TaskRunnerWidget::appendOutput);
    connect(&processRunner_, &TaskProcessRunner::finished, this, [this](int exitCode) {
        appendOutput(QStringLiteral("\n进程结束，退出码：%1\n").arg(exitCode));
        setTaskFinished(exitCode);
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
    if (!runButton_->isEnabled()) {
        appendOutput(QStringLiteral("\n已有任务在运行，请先停止再执行其他命令。\n"));
        return;
    }
    const auto taskIndex = taskSelector_->currentIndex();
    if (taskIndex < 0 || taskIndex >= taskConfig_.tasks.size()) {
        outputView_->setPlainText(QStringLiteral("未选择任务。"));
        return;
    }

    outputView_->clear();
    taskOutputBuffer_.clear();
    taskDiagnostics_.clear();
    const auto &task = taskConfig_.tasks.at(taskIndex);
    const auto request = TaskExecutionRequest::fromTask(task, workspaceRoot_);
    stopRequested_ = false;
    if (!processRunner_.start(request)) {
        setTaskRunning(false);
        outputView_->setPlainText(QStringLiteral("无法启动任务。"));
        return;
    }

    setTaskRunning(true, task.name);
}

void TaskRunnerWidget::stopRunningTask()
{
    stopRequested_ = true;
    processRunner_.stop();
}

void TaskRunnerWidget::appendOutput(const QString &output)
{
    taskOutputBuffer_.append(output);
    outputView_->moveCursor(QTextCursor::End);
    outputView_->insertPlainText(output);
    outputView_->moveCursor(QTextCursor::End);
}

void TaskRunnerWidget::setTaskRunning(bool isRunning, const QString &taskName)
{
    runButton_->setEnabled(!isRunning);
    terminalRunButton_->setEnabled(!isRunning);
    stopButton_->setEnabled(isRunning);
    statusLabel_->setText(isRunning ? QStringLiteral("运行中：%1").arg(taskName) : QStringLiteral("空闲"));
}

void TaskRunnerWidget::setTaskFinished(int exitCode)
{
    runButton_->setEnabled(true);
    stopButton_->setEnabled(false);

    if (stopRequested_) {
        stopRequested_ = false;
        statusLabel_->setText(QStringLiteral("空闲"));
        return;
    }

    appendDiagnosticSummary();
    statusLabel_->setText(exitCode == 0
                              ? QStringLiteral("成功")
                              : QStringLiteral("失败：退出码 %1").arg(exitCode));
}

void TaskRunnerWidget::appendDiagnosticSummary()
{
    taskDiagnostics_ = TaskDiagnosticParser::parse(taskOutputBuffer_);
    if (taskDiagnostics_.isEmpty()) {
        return;
    }

    outputView_->moveCursor(QTextCursor::End);
    outputView_->insertPlainText(QStringLiteral("\n诊断信息：\n"));
    for (int index = 0; index < taskDiagnostics_.size(); ++index) {
        const auto &diagnostic = taskDiagnostics_.at(index);
        const auto line = QStringLiteral("%1: %2:%3:%4: %5")
                              .arg(diagnostic.severity)
                              .arg(diagnostic.filePath)
                              .arg(diagnostic.line)
                              .arg(diagnostic.column)
                              .arg(diagnostic.message);
        outputView_->insertHtml(QStringLiteral("<a href=\"toide-diagnostic:%1\">%2</a><br>")
                                    .arg(index)
                                    .arg(line.toHtmlEscaped()));
    }
    outputView_->moveCursor(QTextCursor::End);
}
