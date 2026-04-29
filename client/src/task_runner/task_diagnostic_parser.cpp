#include "task_runner/task_diagnostic_parser.h"

#include <QRegularExpression>

QVector<TaskDiagnostic> TaskDiagnosticParser::parse(const QString &output)
{
    QVector<TaskDiagnostic> diagnostics;
    const auto lines = output.split(QLatin1Char('\n'));

    const QRegularExpression gccPattern(
        QStringLiteral(R"(^(.+):(\d+):(\d+):\s+(error|warning|note):\s+(.+)$)"));
    const QRegularExpression msvcPattern(
        QStringLiteral(R"(^(.+)\((\d+)\):\s+(error|warning|note)\s+(.+)$)"));

    for (const auto &line : lines) {
        auto match = gccPattern.match(line);
        if (match.hasMatch()) {
            TaskDiagnostic diagnostic;
            diagnostic.filePath = match.captured(1);
            diagnostic.line = match.captured(2).toInt();
            diagnostic.column = match.captured(3).toInt();
            diagnostic.severity = match.captured(4);
            diagnostic.message = match.captured(5);
            diagnostics.append(diagnostic);
            continue;
        }

        match = msvcPattern.match(line);
        if (match.hasMatch()) {
            TaskDiagnostic diagnostic;
            diagnostic.filePath = match.captured(1);
            diagnostic.line = match.captured(2).toInt();
            diagnostic.severity = match.captured(3);
            diagnostic.message = match.captured(4);
            diagnostics.append(diagnostic);
        }
    }

    return diagnostics;
}
