#pragma once

#include <QWidget>

#include "settings/server_endpoint_settings.h"

class QLabel;
class QLineEdit;
class QPushButton;

class CollaborationPanelWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationPanelWidget(QWidget *parent = nullptr,
                                      const QString &endpointSettingsIniPath = QString());

    void setServerStatus(bool online, const QString &message);

signals:
    void serverHealthCheckRequested(const QUrl &serverBaseUrl);

private:
    ServerEndpointSettings endpointSettings_;
    QLabel *serverConnectionStatusLabel_ = nullptr;
    QLineEdit *serverBaseUrlEdit_ = nullptr;
    QPushButton *checkServerConnectionButton_ = nullptr;
};
