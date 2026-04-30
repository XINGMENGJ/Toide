#pragma once

#include <QWidget>

#include "settings/server_endpoint_settings.h"

class QLabel;
class QLineEdit;
class QPushButton;

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
class CollaborationWebSocketClient;
#endif

class CollaborationPanelWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationPanelWidget(QWidget *parent = nullptr,
                                      const QString &endpointSettingsIniPath = QString());

    void setServerStatus(bool online, const QString &message);
    void setWorkspaceKey(const QString &workspacePath);

signals:
    void serverHealthCheckRequested(const QUrl &serverBaseUrl);

private:
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    void updateCollaborationChannelUi();
    void onCollaborationChannelButtonClicked();
#endif

    ServerEndpointSettings endpointSettings_;
    QString collaborationProjectId_{QStringLiteral("default")};
    QLabel *serverConnectionStatusLabel_ = nullptr;
    QLineEdit *serverBaseUrlEdit_ = nullptr;
    QPushButton *checkServerConnectionButton_ = nullptr;
    QLabel *collaborationChannelStatusLabel_ = nullptr;
    QPushButton *collaborationChannelButton_ = nullptr;
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    CollaborationWebSocketClient *collaborationWsClient_ = nullptr;
#endif
};
