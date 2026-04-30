#include "git/git_status_widget.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QThread>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

bool copyTextToClipboard(const QString &text)
{
    auto *clipboard = QGuiApplication::clipboard();
    for (int attempt = 0; attempt < 10; ++attempt) {
        clipboard->setText(text);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        if (clipboard->text() == text) {
            return true;
        }

        QThread::msleep(25);
    }

    return false;
}

} // namespace

GitStatusWidget::GitStatusWidget(QWidget *parent)
    : QWidget(parent)
    , introLabel_(new QLabel(this))
    , refreshButton_(new QPushButton(QStringLiteral("刷新状态"), this))
    , copyButton_(new QPushButton(QStringLiteral("复制状态"), this))
    , copyBranchButton_(new QPushButton(QStringLiteral("复制分支名"), this))
    , openTerminalButton_(new QPushButton(QStringLiteral("打开终端"), this))
    , refreshStatusLabel_(new QLabel(QStringLiteral("尚未刷新"), this))
    , statusView_(new QTextEdit(this))
{
    refreshButton_->setObjectName(QStringLiteral("refreshGitStatusButton"));
    copyButton_->setObjectName(QStringLiteral("copyGitStatusButton"));
    copyBranchButton_->setObjectName(QStringLiteral("copyGitBranchButton"));
    openTerminalButton_->setObjectName(QStringLiteral("openGitTerminalButton"));
    refreshStatusLabel_->setObjectName(QStringLiteral("gitRefreshStatusLabel"));
    statusView_->setObjectName(QStringLiteral("gitStatusView"));
    introLabel_->setObjectName(QStringLiteral("gitIntroLabel"));
    copyBranchButton_->setEnabled(false);
    statusView_->setReadOnly(true);

    introLabel_->setWordWrap(true);
    introLabel_->setTextFormat(Qt::RichText);
    introLabel_->setText(
        QStringLiteral("<p style='margin:0 0 6px 0'><b>Git</b><br/>"
                       "<span style='color:#444'>读取当前工作区根目录的仓库状态（命令：<code>git status --short "
                       "--branch</code>）。打开项目时会自动刷新一次。需要本机已安装 <code>git</code> 且该目录为 Git 仓库。</span></p>"));

    auto *toolbarLayout = new QHBoxLayout;
    toolbarLayout->addWidget(refreshStatusLabel_, 0, Qt::AlignVCenter);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(openTerminalButton_);
    toolbarLayout->addWidget(copyBranchButton_);
    toolbarLayout->addWidget(copyButton_);
    toolbarLayout->addWidget(refreshButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(introLabel_);
    layout->addLayout(toolbarLayout);
    layout->addWidget(statusView_, 1);

    connect(refreshButton_, &QPushButton::clicked, this, [this]() {
        loadStatusFromWorkspace(workspaceRoot_);
    });
    connect(copyButton_, &QPushButton::clicked, this, [this]() {
        const auto statusText = statusView_->toPlainText();
        if (statusText.isEmpty()) {
            refreshStatusLabel_->setText(QStringLiteral("没有可复制的状态"));
            return;
        }

        refreshStatusLabel_->setText(copyTextToClipboard(statusText)
                                         ? QStringLiteral("已复制状态")
                                         : QStringLiteral("无法复制状态"));
    });
    connect(copyBranchButton_, &QPushButton::clicked, this, [this]() {
        if (currentBranchName_.isEmpty()) {
            refreshStatusLabel_->setText(QStringLiteral("没有可复制的分支"));
            return;
        }

        refreshStatusLabel_->setText(copyTextToClipboard(currentBranchName_)
                                         ? QStringLiteral("已复制分支名")
                                         : QStringLiteral("无法复制分支名"));
    });
    connect(openTerminalButton_, &QPushButton::clicked, this, [this]() {
        if (workspaceRoot_.isEmpty()) {
            refreshStatusLabel_->setText(QStringLiteral("未打开工作区。"));
            return;
        }

        refreshStatusLabel_->setText(QStringLiteral("正在打开终端"));
        emit openTerminalRequested(workspaceRoot_);
    });
}

bool GitStatusWidget::loadStatusFromWorkspace(const QString &workspaceRoot)
{
    workspaceRoot_ = workspaceRoot;
    currentBranchName_.clear();
    copyBranchButton_->setEnabled(false);
    if (workspaceRoot_.isEmpty()) {
        refreshStatusLabel_->setText(QStringLiteral("未打开工作区。"));
        statusView_->setPlainText(QStringLiteral("未打开工作区。"));
        return false;
    }

    QProcess process;
    process.setWorkingDirectory(workspaceRoot_);
    process.start(QStringLiteral("git"), QStringList{QStringLiteral("status"), QStringLiteral("--short"), QStringLiteral("--branch")});
    if (!process.waitForFinished(3000)) {
        process.kill();
        refreshStatusLabel_->setText(QStringLiteral("git status 超时。"));
        statusView_->setPlainText(QStringLiteral("git status 超时。"));
        return false;
    }

    const auto output = QString::fromLocal8Bit(process.readAllStandardOutput());
    const auto errorOutput = QString::fromLocal8Bit(process.readAllStandardError());
    if (process.exitCode() != 0) {
        refreshStatusLabel_->setText(QStringLiteral("不是 Git 仓库。"));
        statusView_->setPlainText(QStringLiteral("不是 Git 仓库。\n%1").arg(errorOutput.trimmed()));
        return false;
    }

    currentBranchName_ = parseBranchName(output);
    copyBranchButton_->setEnabled(!currentBranchName_.isEmpty());
    refreshStatusLabel_->setText(QStringLiteral("已刷新"));
    statusView_->setPlainText(formatStatusOutput(output));
    return true;
}

QString GitStatusWidget::parseBranchName(const QString &statusOutput)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto lines = statusOutput.split(QLatin1Char('\n'), QString::SkipEmptyParts);
#else
    const auto lines = statusOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
#endif

    for (const auto &line : lines) {
        if (!line.startsWith(QStringLiteral("##"))) {
            continue;
        }

        const auto branchLine = line.mid(3).trimmed();
        const auto trackingSeparator = branchLine.indexOf(QStringLiteral("..."));
        if (trackingSeparator >= 0) {
            return branchLine.left(trackingSeparator).trimmed();
        }

        return branchLine;
    }

    return {};
}

QString GitStatusWidget::formatStatusOutput(const QString &statusOutput)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto lines = statusOutput.split(QLatin1Char('\n'), QString::SkipEmptyParts);
#else
    const auto lines = statusOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
#endif
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
    formatted.append(QStringLiteral("分支"));
    formatted.append(branchLine.isEmpty() ? QStringLiteral("（未知）") : branchLine);
    formatted.append(QString());

    if (staged.isEmpty() && unstaged.isEmpty() && untracked.isEmpty()) {
        formatted.append(QStringLiteral("工作区干净。"));
        formatted.append(QString());
    } else {
        formatted.append(QStringLiteral("摘要"));
        formatted.append(QStringLiteral("暂存：%1").arg(staged.size()));
        formatted.append(QStringLiteral("未暂存：%1").arg(unstaged.size()));
        formatted.append(QStringLiteral("未跟踪：%1").arg(untracked.size()));
        formatted.append(QString());
    }

    formatted.append(QStringLiteral("暂存区"));
    formatted.append(staged.isEmpty() ? QStringLiteral("（无）") : staged.join(QLatin1Char('\n')));
    formatted.append(QString());

    formatted.append(QStringLiteral("未暂存"));
    formatted.append(unstaged.isEmpty() ? QStringLiteral("（无）") : unstaged.join(QLatin1Char('\n')));
    formatted.append(QString());

    formatted.append(QStringLiteral("未跟踪"));
    formatted.append(untracked.isEmpty() ? QStringLiteral("（无）") : untracked.join(QLatin1Char('\n')));

    return formatted.join(QLatin1Char('\n'));
}
