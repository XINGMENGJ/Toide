#include "git/git_status_widget.h"

#include <QHBoxLayout>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

GitStatusWidget::GitStatusWidget(QWidget *parent)
    : QWidget(parent)
    , refreshButton_(new QPushButton(QStringLiteral("Refresh"), this))
    , statusView_(new QTextEdit(this))
{
    refreshButton_->setObjectName(QStringLiteral("refreshGitStatusButton"));
    statusView_->setObjectName(QStringLiteral("gitStatusView"));
    statusView_->setReadOnly(true);

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(refreshButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(toolbarLayout);
    layout->addWidget(statusView_, 1);

    connect(refreshButton_, &QPushButton::clicked, this, [this]() {
        loadStatusFromWorkspace(workspaceRoot_);
    });
}

bool GitStatusWidget::loadStatusFromWorkspace(const QString &workspaceRoot)
{
    workspaceRoot_ = workspaceRoot;
    if (workspaceRoot_.isEmpty()) {
        statusView_->setPlainText(QStringLiteral("No workspace is open."));
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(workspaceRoot_);
    process.start(QStringLiteral("git"), QStringList{QStringLiteral("status"), QStringLiteral("--short"), QStringLiteral("--branch")});
    if (!process.waitForFinished(3000)) {
        process.kill();
        statusView_->setPlainText(QStringLiteral("Git status timed out."));
        return false;
    }

    const auto output = QString::fromLocal8Bit(process.readAllStandardOutput());
    const auto errorOutput = QString::fromLocal8Bit(process.readAllStandardError());
    if (process.exitCode() != 0) {
        statusView_->setPlainText(QStringLiteral("Not a Git repository.\n%1").arg(errorOutput.trimmed()));
        return false;
    }

    statusView_->setPlainText(output.trimmed().isEmpty() ? QStringLiteral("Working tree clean.") : output);
    return true;
}
