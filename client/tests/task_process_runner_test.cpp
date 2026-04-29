#include <QtTest/QtTest>

#include "task_runner/task_execution_request.h"
#include "task_runner/task_process_runner.h"

#include <QSignalSpy>
#include <QTemporaryDir>

class TaskProcessRunnerTest final : public QObject {
    Q_OBJECT

private slots:
    void runCapturesStandardOutputAndExitCode();
};

void TaskProcessRunnerTest::runCapturesStandardOutputAndExitCode()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    TaskExecutionRequest request;
    request.name = QStringLiteral("Echo");
#ifdef Q_OS_WIN
    request.command = QStringLiteral("echo ToideTask");
#else
    request.command = QStringLiteral("echo ToideTask");
#endif
    request.workingDirectory = directory.path();

    TaskProcessRunner runner;
    QSignalSpy outputSpy(&runner, &TaskProcessRunner::outputReceived);
    QSignalSpy finishedSpy(&runner, &TaskProcessRunner::finished);

    QVERIFY(runner.start(request));
    QVERIFY(finishedSpy.wait(3000));

    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QVERIFY(!outputSpy.isEmpty());
    QVERIFY(outputSpy.takeFirst().at(0).toString().contains(QStringLiteral("ToideTask")));
}

QTEST_MAIN(TaskProcessRunnerTest)

#include "task_process_runner_test.moc"
