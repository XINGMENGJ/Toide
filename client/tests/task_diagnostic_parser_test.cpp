#include <QtTest/QtTest>

#include "task_runner/task_diagnostic_parser.h"

class TaskDiagnosticParserTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesGccStyleDiagnostics();
    void parsesMsvcStyleDiagnostics();
};

void TaskDiagnosticParserTest::parsesGccStyleDiagnostics()
{
    const auto diagnostics = TaskDiagnosticParser::parse(QStringLiteral(
        "src/hello_toide.cpp:5:18: error: expected ';' before 'return'\n"
        "src/hello_toide.cpp:6:5: warning: unused variable 'value'\n"));

    QCOMPARE(diagnostics.size(), 2);
    QCOMPARE(diagnostics.at(0).filePath, QStringLiteral("src/hello_toide.cpp"));
    QCOMPARE(diagnostics.at(0).line, 5);
    QCOMPARE(diagnostics.at(0).column, 18);
    QCOMPARE(diagnostics.at(0).severity, QStringLiteral("error"));
    QCOMPARE(diagnostics.at(0).message, QStringLiteral("expected ';' before 'return'"));
    QCOMPARE(diagnostics.at(1).severity, QStringLiteral("warning"));
}

void TaskDiagnosticParserTest::parsesMsvcStyleDiagnostics()
{
    const auto diagnostics = TaskDiagnosticParser::parse(QStringLiteral(
        "src\\hello_toide.cpp(7): error C2143: syntax error: missing ';' before 'return'\n"));

    QCOMPARE(diagnostics.size(), 1);
    QCOMPARE(diagnostics.at(0).filePath, QStringLiteral("src\\hello_toide.cpp"));
    QCOMPARE(diagnostics.at(0).line, 7);
    QCOMPARE(diagnostics.at(0).column, 0);
    QCOMPARE(diagnostics.at(0).severity, QStringLiteral("error"));
    QCOMPARE(diagnostics.at(0).message, QStringLiteral("C2143: syntax error: missing ';' before 'return'"));
}

QTEST_MAIN(TaskDiagnosticParserTest)

#include "task_diagnostic_parser_test.moc"
