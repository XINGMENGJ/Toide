#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class CollaborationPanelWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationPanelWidget(QWidget *parent = nullptr);

    void setServerStatus(bool online, const QString &message);

signals:
    void serverHealthCheckRequested();

private:
    QLabel *serverConnectionStatusLabel_ = nullptr;
    QPushButton *checkServerConnectionButton_ = nullptr;
};
