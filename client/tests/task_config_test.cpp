#include <QtTest/QtTest>

#include "task_runner/task_config.h"

#include <QTemporaryDir>

class TaskConfigTest final : public QObject {
    Q_OBJECT

private slots:
    void loadFromFileParsesTasks();
    void loadFromFileRejectsMissingCommand();
    void defaultWorkspaceProvidesBuildAndRunTasks();
    void defaultWorkspaceBuildTasksInitializeCompilerEnvironment();
    void defaultWorkspaceProvidesDiagnosticDemoTaskAndFile();
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

void TaskConfigTest::defaultWorkspaceProvidesBuildAndRunTasks()
{
#ifdef TOIDE_SOURCE_DIR
    const auto configPath = QStringLiteral(TOIDE_SOURCE_DIR) + QStringLiteral("/examples/default-workspace/.toide/tasks.json");

    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(configPath, &errorMessage);

    QVERIFY2(config.has_value(), qPrintable(errorMessage));

    QStringList taskNames;
    for (const auto &task : config->tasks) {
        taskNames.append(task.name);
    }

    QVERIFY(taskNames.contains(QStringLiteral("Build Example")));
    QVERIFY(taskNames.contains(QStringLiteral("Run Example")));
#else
    QFAIL("TOIDE_SOURCE_DIR must be defined by the build system.");
#endif
}

void TaskConfigTest::defaultWorkspaceBuildTasksInitializeCompilerEnvironment()
{
#ifdef TOIDE_SOURCE_DIR
    const auto configPath = QStringLiteral(TOIDE_SOURCE_DIR) + QStringLiteral("/examples/default-workspace/.toide/tasks.json");

    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(configPath, &errorMessage);
    QVERIFY2(config.has_value(), qPrintable(errorMessage));

    bool buildUsesQtEnvironment = false;
    bool buildAlwaysCompilesWhenBuildDirectoryExists = false;
    bool runChecksMissingExecutable = false;
    bool diagnosticsUsesQtEnvironment = false;
    bool diagnosticsAlwaysCompilesWhenBuildDirectoryExists = false;

    for (const auto &task : config->tasks) {
        if (task.name == QStringLiteral("Build Example")) {
            buildUsesQtEnvironment = task.command.contains(QStringLiteral("qt6.7-env.cmd"))
                && task.command.contains(QStringLiteral("-std=gnu++2a"));
            buildAlwaysCompilesWhenBuildDirectoryExists = task.command.contains(QStringLiteral("(mkdir build 2>nul & where g++"))
                && !task.command.contains(QStringLiteral("if not exist build mkdir build &&"));
        } else if (task.name == QStringLiteral("Run Example")) {
            runChecksMissingExecutable = task.command.contains(QStringLiteral("if exist build\\hello_toide.exe"))
                && task.command.contains(QStringLiteral("Run Build Example first"))
                && task.command.contains(QStringLiteral("qt6.7-env.cmd"));
        } else if (task.name == QStringLiteral("Build Diagnostics Demo")) {
            diagnosticsUsesQtEnvironment = task.command.contains(QStringLiteral("qt6.7-env.cmd"))
                && task.command.contains(QStringLiteral("-std=gnu++2a"));
            diagnosticsAlwaysCompilesWhenBuildDirectoryExists = task.command.contains(QStringLiteral("(mkdir build 2>nul & where g++"))
                && !task.command.contains(QStringLiteral("if not exist build mkdir build &&"));
        }
    }

    QVERIFY(buildUsesQtEnvironment);
    QVERIFY(buildAlwaysCompilesWhenBuildDirectoryExists);
    QVERIFY(runChecksMissingExecutable);
    QVERIFY(diagnosticsUsesQtEnvironment);
    QVERIFY(diagnosticsAlwaysCompilesWhenBuildDirectoryExists);
#else
    QFAIL("TOIDE_SOURCE_DIR must be defined by the build system.");
#endif
}

void TaskConfigTest::defaultWorkspaceProvidesDiagnosticDemoTaskAndFile()
{
#ifdef TOIDE_SOURCE_DIR
    const auto workspaceRoot = QStringLiteral(TOIDE_SOURCE_DIR) + QStringLiteral("/examples/default-workspace");
    const auto configPath = workspaceRoot + QStringLiteral("/.toide/tasks.json");

    QVERIFY(QFileInfo::exists(workspaceRoot + QStringLiteral("/src/diagnostic_demo.cpp")));

    QString errorMessage;
    const auto config = TaskConfig::loadFromFile(configPath, &errorMessage);
    QVERIFY2(config.has_value(), qPrintable(errorMessage));

    bool foundDiagnosticTask = false;
    for (const auto &task : config->tasks) {
        if (task.name == QStringLiteral("Build Diagnostics Demo")) {
            foundDiagnosticTask = task.command.contains(QStringLiteral("diagnostic_demo.cpp"));
            break;
        }
    }

    QVERIFY(foundDiagnosticTask);
#else
    QFAIL("TOIDE_SOURCE_DIR must be defined by the build system.");
#endif
}

QTEST_MAIN(TaskConfigTest)

#include "task_config_test.moc"
