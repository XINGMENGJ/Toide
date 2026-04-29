#include <QtTest/QtTest>

#include "task_runner/task_config.h"

#include <QTemporaryDir>

class TaskConfigTest final : public QObject {
    Q_OBJECT

private slots:
    void loadFromFileParsesTasks();
    void loadFromFileRejectsMissingCommand();
};

void TaskConfigTest::loadFromFileParsesTasks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto configPath = directory.filePath(QStringLiteral("tasks.json"));
    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
  "tasks": [
    {
      "name": "Build",
      "command": "cmake --build build",
      "workingDirectory": "${workspaceRoot}"
    }
  ]
})");
    file.close();

    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(configPath, &errorMessage);

    QVERIFY2(config.has_value(), qPrintable(errorMessage));
    QCOMPARE(config->tasks.size(), 1);
    QCOMPARE(config->tasks.at(0).name, QStringLiteral("Build"));
    QCOMPARE(config->tasks.at(0).command, QStringLiteral("cmake --build build"));
    QCOMPARE(config->tasks.at(0).workingDirectory, QStringLiteral("${workspaceRoot}"));
}

void TaskConfigTest::loadFromFileRejectsMissingCommand()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto configPath = directory.filePath(QStringLiteral("tasks.json"));
    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
  "tasks": [
    {
      "name": "Build"
    }
  ]
})");
    file.close();

    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(configPath, &errorMessage);

    QVERIFY(!config.has_value());
    QVERIFY(errorMessage.contains(QStringLiteral("command")));
}

QTEST_MAIN(TaskConfigTest)

#include "task_config_test.moc"
