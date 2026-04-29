#pragma once

#include <QString>
#include <QVector>

struct TaskDiagnostic {
    QString filePath;
    int line = 0;
    int column = 0;
    QString severity;
    QString message;
};

class TaskDiagnosticParser final {
public:
    static QVector<TaskDiagnostic> parse(const QString &output);
};
