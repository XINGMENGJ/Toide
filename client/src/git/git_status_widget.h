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

signals:
    void openTerminalRequested(const QString &workspaceRoot);

private:
    static QString formatStatusOutput(const QString &statusOutput);
    static QString parseBranchName(const QString &statusOutput);

    QString workspaceRoot_;
    QString currentBranchName_;
    QPushButton *refreshButton_ = nullptr;
    QPushButton *copyButton_ = nullptr;
    QPushButton *copyBranchButton_ = nullptr;
    QPushButton *openTerminalButton_ = nullptr;
    QLabel *refreshStatusLabel_ = nullptr;
    QTextEdit *statusView_ = nullptr;
};
