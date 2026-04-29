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
    void openTerminalRequestsCurrentWorkspace();
    void loadStatusFromNonGitWorkspaceShowsHelpfulMessage();
};

static bool runGit(const QString &workingDirectory, const QStringList &arguments)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStringLiteral("git"), arguments);
    return process.waitForFinished(3000) && process.exitCode() == 0;
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
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("Branch")));
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
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("Working tree clean.")));
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
    QVERIFY(text.contains(QStringLiteral("Branch")));
    QVERIFY(text.contains(QStringLiteral("Staged")));
    QVERIFY(text.contains(QStringLiteral("Unstaged")));
    QVERIFY(text.contains(QStringLiteral("Untracked")));
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
    QVERIFY(text.contains(QStringLiteral("Summary")));
    QVERIFY(text.contains(QStringLiteral("Staged: 1")));
    QVERIFY(text.contains(QStringLiteral("Unstaged: 1")));
    QVERIFY(text.contains(QStringLiteral("Untracked: 1")));
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
    QVERIFY(statusLabel->text().contains(QStringLiteral("Refreshed")));

    QTemporaryDir nonGitWorkspace;
    QVERIFY(nonGitWorkspace.isValid());
    QVERIFY(!widget.loadStatusFromWorkspace(nonGitWorkspace.path()));
    QVERIFY(statusLabel->text().contains(QStringLiteral("Not a Git repository")));
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

void GitStatusWidgetTest::loadStatusFromNonGitWorkspaceShowsHelpfulMessage()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    GitStatusWidget widget;
    QVERIFY(!widget.loadStatusFromWorkspace(workspace.path()));

    auto *statusView = widget.findChild<QTextEdit *>(QStringLiteral("gitStatusView"));
    QVERIFY(statusView != nullptr);
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("Not a Git repository")));
}

QTEST_MAIN(GitStatusWidgetTest)

#include "git_status_widget_test.moc"
