#include <QtTest/QtTest>

#include "git/git_status_widget.h"

#include <QProcess>
#include <QTemporaryDir>
#include <QTextEdit>

class GitStatusWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void loadStatusFromGitWorkspaceShowsBranchAndChanges();
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
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("##")));
    QVERIFY(statusView->toPlainText().contains(QStringLiteral("notes.txt")));
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
