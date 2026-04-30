#pragma once

#include <QHash>
#include <QString>
#include <QWidget>

#include "settings/server_endpoint_settings.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
class CollaborationWebSocketClient;
#endif

struct CollaboratorPeer {
    QString clientId;
    QString currentFile;
};

class CollaborationPanelWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationPanelWidget(QWidget *parent = nullptr,
                                      const QString &endpointSettingsIniPath = QString());

    void setServerStatus(bool online, const QString &message);
    void setWorkspaceKey(const QString &workspacePath);
    void setWorkspaceRoot(const QString &absoluteRootPath);

    void notifyCurrentFile(const QString &absoluteFilePath);
    void notifyLocalFileSaved(const QString &absoluteFilePath);

signals:
    void serverHealthCheckRequested(const QUrl &serverBaseUrl);
    void collaborationRosterSynced();

private:
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    void updateCollaborationChannelUi();
    void onCollaborationChannelButtonClicked();
    void onWebSocketMessage(const QString &text);
    void sendPresenceJoin();
    void clearPresenceUi();
    void applyRosterJson(const QJsonObject &obj);
    void applyUserJoined(const QJsonObject &obj);
    void applyUserLeft(const QJsonObject &obj);
    void applyCurrentFileChanged(const QJsonObject &obj);
    void rebuildOnlineList();
    void appendActivityLine(const QString &line);
    QString relativeWorkspacePath(const QString &absolutePath) const;
    QString displayNameForClient(const QString &clientId) const;
#endif

    ServerEndpointSettings endpointSettings_;
    QString collaborationProjectId_{QStringLiteral("default")};
    QString workspaceRootAbsolute_;

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    QString localClientId_;
    QHash<QString, CollaboratorPeer> remotePeers_;
#endif

    QLabel *serverConnectionStatusLabel_ = nullptr;
    QLineEdit *serverBaseUrlEdit_ = nullptr;
    QPushButton *checkServerConnectionButton_ = nullptr;
    QLabel *collaborationChannelStatusLabel_ = nullptr;
    QPushButton *collaborationChannelButton_ = nullptr;
    QLabel *onlineMembersCaption_ = nullptr;
    QListWidget *onlineMembersList_ = nullptr;
    QLabel *activityCaption_ = nullptr;
    QListWidget *activityList_ = nullptr;
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    CollaborationWebSocketClient *collaborationWsClient_ = nullptr;
#endif
};
