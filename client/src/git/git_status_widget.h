#pragma once

#include <QWidget>

class QPushButton;
class QLabel;
class QTextEdit;

class GitStatusWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GitStatusWidget(QWidget *parent = nullptr);

    bool loadStatusFromWorkspace(const QString &workspaceRoot);

private:
    static QString formatStatusOutput(const QString &statusOutput);

    QString workspaceRoot_;
    QPushButton *refreshButton_ = nullptr;
    QLabel *refreshStatusLabel_ = nullptr;
    QTextEdit *statusView_ = nullptr;
};
