#include "git/git_status_widget.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

GitStatusWidget::GitStatusWidget(QWidget *parent)
    : QWidget(parent)
    , refreshButton_(new QPushButton(QStringLiteral("Refresh"), this))
    , copyButton_(new QPushButton(QStringLiteral("Copy status"), this))
    , openTerminalButton_(new QPushButton(QStringLiteral("Open terminal"), this))
    , refreshStatusLabel_(new QLabel(QStringLiteral("Not refreshed"), this))
    , statusView_(new QTextEdit(this))
{
    refreshButton_->setObjectName(QStringLiteral("refreshGitStatusButton"));
    copyButton_->setObjectName(QStringLiteral("copyGitStatusButton"));
    openTerminalButton_->setObjectName(QStringLiteral("openGitTerminalButton"));
    refreshStatusLabel_->setObjectName(QStringLiteral("gitRefreshStatusLabel"));
    statusView_->setObjectName(QStringLiteral("gitStatusView"));
    statusView_->setReadOnly(true);

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->addWidget(refreshStatusLabel_, 1);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(openTerminalButton_);
    toolbarLayout->addWidget(copyButton_);
    toolbarLayout->addWidget(refreshButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(toolbarLayout);
    layout->addWidget(statusView_, 1);

    connect(refreshButton_, &QPushButton::clicked, this, [this]() {
        loadStatusFromWorkspace(workspaceRoot_);
    });
    connect(copyButton_, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(statusView_->toPlainText());
    });
    connect(openTerminalButton_, &QPushButton::clicked, this, [this]() {
        if (workspaceRoot_.isEmpty()) {
            refreshStatusLabel_->setText(QStringLiteral("No workspace is open."));
            return;
        }

        emit openTerminalRequested(workspaceRoot_);
    });
}

bool GitStatusWidget::loadStatusFromWorkspace(const QString &workspaceRoot)
{
    workspaceRoot_ = workspaceRoot;
    if (workspaceRoot_.isEmpty()) {
        refreshStatusLabel_->setText(QStringLiteral("No workspace is open."));
        statusView_->setPlainText(QStringLiteral("No workspace is open."));
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(workspaceRoot_);
    process.start(QStringLiteral("git"), QStringList{QStringLiteral("status"), QStringLiteral("--short"), QStringLiteral("--branch")});
    if (!process.waitForFinished(3000)) {
        process.kill();
        refreshStatusLabel_->setText(QStringLiteral("Git status timed out."));
        statusView_->setPlainText(QStringLiteral("Git status timed out."));
        return false;
    }

    const auto output = QString::fromLocal8Bit(process.readAllStandardOutput());
    const auto errorOutput = QString::fromLocal8Bit(process.readAllStandardError());
    if (process.exitCode() != 0) {
        refreshStatusLabel_->setText(QStringLiteral("Not a Git repository."));
        statusView_->setPlainText(QStringLiteral("Not a Git repository.\n%1").arg(errorOutput.trimmed()));
        return false;
    }

    refreshStatusLabel_->setText(QStringLiteral("Refreshed"));
    statusView_->setPlainText(formatStatusOutput(output));
    return true;
}

QString GitStatusWidget::formatStatusOutput(const QString &statusOutput)
{
    const auto lines = statusOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QString branchLine;
    QStringList staged;
    QStringList unstaged;
    QStringList untracked;

    for (const auto &line : lines) {
        if (line.startsWith(QStringLiteral("##"))) {
            branchLine = line.mid(3).trimmed();
            continue;
        }

        if (line.size() < 3) {
            continue;
        }

        const auto indexStatus = line.at(0);
        const auto workingTreeStatus = line.at(1);
        const auto filePath = line.mid(3).trimmed();

        if (indexStatus == QLatin1Char('?') && workingTreeStatus == QLatin1Char('?')) {
            untracked.append(filePath);
            continue;
        }

        if (indexStatus != QLatin1Char(' ')) {
            staged.append(QStringLiteral("%1 %2").arg(indexStatus, filePath));
        }

        if (workingTreeStatus != QLatin1Char(' ')) {
            unstaged.append(QStringLiteral("%1 %2").arg(workingTreeStatus, filePath));
        }
    }

    QStringList formatted;
    formatted.append(QStringLiteral("Branch"));
    formatted.append(branchLine.isEmpty() ? QStringLiteral("(unknown)") : branchLine);
    formatted.append(QString());

    if (staged.isEmpty() && unstaged.isEmpty() && untracked.isEmpty()) {
        formatted.append(QStringLiteral("Working tree clean."));
        formatted.append(QString());
    }

    formatted.append(QStringLiteral("Staged"));
    formatted.append(staged.isEmpty() ? QStringLiteral("(none)") : staged.join(QLatin1Char('\n')));
    formatted.append(QString());

    formatted.append(QStringLiteral("Unstaged"));
    formatted.append(unstaged.isEmpty() ? QStringLiteral("(none)") : unstaged.join(QLatin1Char('\n')));
    formatted.append(QString());

    formatted.append(QStringLiteral("Untracked"));
    formatted.append(untracked.isEmpty() ? QStringLiteral("(none)") : untracked.join(QLatin1Char('\n')));

    return formatted.join(QLatin1Char('\n'));
}
