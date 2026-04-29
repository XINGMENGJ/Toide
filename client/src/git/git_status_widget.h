#pragma once

#include <QWidget>

class QPushButton;
class QTextEdit;

class GitStatusWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GitStatusWidget(QWidget *parent = nullptr);

    bool loadStatusFromWorkspace(const QString &workspaceRoot);

private:
    QString workspaceRoot_;
    QPushButton *refreshButton_ = nullptr;
    QTextEdit *statusView_ = nullptr;
};
