#include <QtTest/QtTest>

#include "git/git_status_widget.h"

#include <QClipboard>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextEdit>

class GitStatusWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void loadStatusFromGitWorkspaceShowsBranchAndChanges();
    void loadStatusFromCleanGitWorkspaceShowsCleanMessage();
    void loadStatusFromGitWorkspaceGroupsChanges();
    void loadStatusFromGitWorkspaceShowsChangeSummaryCounts();
    void loadStatusShowsRefreshResult();
    void copyStatusCopiesCurrentStatusToClipboard();
    void copyStatusShowsCopiedFeedback();
    void copyStatusWithoutStatusShowsHelpfulFeedback();
    void copyBranchCopiesCurrentBranchToClipboard();
    void copyBranchIsOnlyEnabledWhenBranchIsAvailable();
    void openTerminalRequestsCurrentWorkspace();
    void openTerminalShowsOpeningFeedback();
    void loadStatusFromNonGitWorkspaceShowsHelpfulMessage();
};

static bool runGit(const QString &workingDirectory, const QStringList &arguments)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStringLiteral("git"), arguments);
    return process.waitForFinished(3000) && process.exitCode() == 0;
}

static bool setClipboardTextForTest(const QString &text)
{
    auto *clipboard = QGuiApplication::clipboard();
    for (int attempt = 0; attempt < 20; ++attempt) {
        clipboard->setText(text);
        if (clipboard->text() == text) {
            return true;
        }

        QTest::qWait(50);
    }

    return false;
}

void GitStatusWidgetTest::loadStatusFromGitWorkspaceShowsBranchAndChanges()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    QFile file(workspace.filePath(QStringLiteral("notes.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("hello\n");
    file.close();

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("分支")));
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("notes.txt")));
}

void GitStatusWidgetTest::loadStatusFromCleanGitWorkspaceShowsCleanMessage()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("toide@example.com")}));
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Toide Test")}));

    QFile file(workspace.filePath(QStringLiteral("tracked.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("tracked\n");
    file.close();
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("add"), QStringLiteral("tracked.txt")}));
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")}));

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("工作区干净。")));
}

void GitStatusWidgetTest::loadStatusFromGitWorkspaceGroupsChanges()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    QFile stagedFile(workspace.filePath(QStringLiteral("staged.txt")));
    QVERIFY(stagedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    stagedFile.write("staged\n");
    stagedFile.close();
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("add"), QStringLiteral("staged.txt")}));

    QFile unstagedFile(workspace.filePath(QStringLiteral("staged.txt")));
    QVERIFY(unstagedFile.open(QIODevice::Append | QIODevice::Text));
    unstagedFile.write("unstaged\n");
    unstagedFile.close();

    QFile untrackedFile(workspace.filePath(QStringLiteral("untracked.txt")));
    QVERIFY(untrackedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    untrackedFile.write("untracked\n");
    untrackedFile.close();

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);

    const auto text = statusView->toPlainText();
    QVERIFY(text.contains(QStringLiteral("分支")));
    QVERIFY(text.contains(QStringLiteral("暂存区")));
    QVERIFY(text.contains(QStringLiteral("未暂存")));
    QVERIFY(text.contains(QStringLiteral("未跟踪")));
    QVERIFY(text.contains(QStringLiteral("staged.txt")));
    QVERIFY(text.contains(QStringLiteral("untracked.txt")));
}

void GitStatusWidgetTest::loadStatusFromGitWorkspaceShowsChangeSummaryCounts()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    QFile stagedFile(workspace.filePath(QStringLiteral("staged.txt")));
    QVERIFY(stagedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    stagedFile.write("staged\n");
    stagedFile.close();
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("add"), QStringLiteral("staged.txt")}));

    QFile unstagedFile(workspace.filePath(QStringLiteral("staged.txt")));
    QVERIFY(unstagedFile.open(QIODevice::Append | QIODevice::Text));
    unstagedFile.write("unstaged\n");
    unstagedFile.close();

    QFile untrackedFile(workspace.filePath(QStringLiteral("untracked.txt")));
    QVERIFY(untrackedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    untrackedFile.write("untracked\n");
    untrackedFile.close();

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);

    const auto text = statusView->toPlainText();
    QVERIFY(text.contains(QStringLiteral("摘要")));
    QVERIFY(text.contains(QStringLiteral("暂存：1")));
    QVERIFY(text.contains(QStringLiteral("未暂存：1")));
    QVERIFY(text.contains(QStringLiteral("未跟踪：1")));
}

void GitStatusWidgetTest::loadStatusShowsRefreshResult()
{
    QTemporaryDir gitWorkspace;
    QVERIFY(gitWorkspace.isValid());
    QVERIFY(runGit(gitWorkspace.path(), QStringList{QStringLiteral("init")}));

    GitStatusWidget widget;
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("gitRefreshStatusLabel"));
    QVERIFY(statusLabel != nullptr);

    QVERIFY(widget.loadStatusFromWorkspace(gitWorkspace.path()));
    QVERIFY(statusLabel->text().contains(QStringLiteral("已刷新")));

    QTemporaryDir nonGitWorkspace;
    QVERIFY(nonGitWorkspace.isValid());
    QVERIFY(!widget.loadStatusFromWorkspace(nonGitWorkspace.path()));
    QVERIFY(statusLabel->text().contains(QStringLiteral("不是 Git 仓库")));
}

void GitStatusWidgetTest::copyStatusCopiesCurrentStatusToClipboard()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    QFile file(workspace.filePath(QStringLiteral("notes.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("hello\n");
    file.close();

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    auto *copyButton = widget.findChild<QPushButton *>(QStringLiteral("copyGitStatusButton"));
    QVERIFY(copyButton != nullptr);

    QGuiApplication::clipboard()->clear();
    copyButton->click();

    QCOMPARE(QGuiApplication::clipboard()->text(), statusView->toPlainText());
    QVERIFY(QGuiApplication::clipboard()->text().contains(QStringLiteral("notes.txt")));
}

void GitStatusWidgetTest::copyStatusShowsCopiedFeedback()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("gitRefreshStatusLabel"));
    QVERIFY(statusLabel != nullptr);
    auto *copyButton = widget.findChild<QPushButton *>(QStringLiteral("copyGitStatusButton"));
    QVERIFY(copyButton != nullptr);

    copyButton->click();

    QVERIFY(statusLabel->text().contains(QStringLiteral("已复制状态")));
}

void GitStatusWidgetTest::copyStatusWithoutStatusShowsHelpfulFeedback()
{
    GitStatusWidget widget;
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("gitRefreshStatusLabel"));
    QVERIFY(statusLabel != nullptr);
    auto *copyButton = widget.findChild<QPushButton *>(QStringLiteral("copyGitStatusButton"));
    QVERIFY(copyButton != nullptr);

    QVERIFY(setClipboardTextForTest(QStringLiteral("keep me")));
    copyButton->click();

    QVERIFY(statusLabel->text().contains(QStringLiteral("没有可复制的状态")));
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("keep me"));
}

void GitStatusWidgetTest::copyBranchCopiesCurrentBranchToClipboard()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    const auto statusText = statusView->toPlainText();
    QVERIFY(statusText.contains(QStringLiteral("分支")));

    auto *copyBranchButton = widget.findChild<QPushButton *>(QStringLiteral("copyGitBranchButton"));
    QVERIFY(copyBranchButton != nullptr);
    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("gitRefreshStatusLabel"));
    QVERIFY(statusLabel != nullptr);

    copyBranchButton->click();

    QVERIFY(!QGuiApplication::clipboard()->text().isEmpty());
    QVERIFY(statusText.contains(QGuiApplication::clipboard()->text()));
    QVERIFY(statusLabel->text().contains(QStringLiteral("已复制分支名")));
}

void GitStatusWidgetTest::copyBranchIsOnlyEnabledWhenBranchIsAvailable()
{
    GitStatusWidget widget;
    auto *copyBranchButton = widget.findChild<QPushButton *>(QStringLiteral("copyGitBranchButton"));
    QVERIFY(copyBranchButton != nullptr);
    QVERIFY(!copyBranchButton->isEnabled());

    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));
    QVERIFY(copyBranchButton->isEnabled());
}

void GitStatusWidgetTest::openTerminalRequestsCurrentWorkspace()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    QSignalSpy terminalSpy(&widget, SIGNAL(openTerminalRequested(QString)));
    QVERIFY(terminalSpy.isValid());

    auto *openTerminalButton = widget.findChild<QPushButton *>(QStringLiteral("openGitTerminalButton"));
    QVERIFY(openTerminalButton != nullptr);

    openTerminalButton->click();

    QCOMPARE(terminalSpy.count(), 1);
    QCOMPARE(terminalSpy.takeFirst().at(0).toString(), workspace.path());
}

void GitStatusWidgetTest::openTerminalShowsOpeningFeedback()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QVERIFY(runGit(workspace.path(), QStringList{QStringLiteral("init")}));

    GitStatusWidget widget;
    QVERIFY(widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("gitRefreshStatusLabel"));
    QVERIFY(statusLabel != nullptr);
    auto *openTerminalButton = widget.findChild<QPushButton *>(QStringLiteral("openGitTerminalButton"));
    QVERIFY(openTerminalButton != nullptr);

    openTerminalButton->click();

    QVERIFY(statusLabel->text().contains(QStringLiteral("正在打开终端")));
}

void GitStatusWidgetTest::loadStatusFromNonGitWorkspaceShowsHelpfulMessage()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    GitStatusWidget widget;
    QVERIFY(!widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("不是 Git 仓库")));
}

QTEST_MAIN(GitStatusWidgetTest)

#include "git_status_widget_test.moc"
