#pragma once

#include <QHash>
#include <QString>
#include <QTimer>
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
    QString username;
};

class CollaborationPanelWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationPanelWidget(QWidget *parent = nullptr,
                                      const QString &endpointSettingsIniPath = QString());

    void setServerStatus(bool online, const QString &message);
    void setWorkspaceKey(const QString &workspacePath);
    void setCollaborationProjectKey(const QString &projectKey);
    void setWorkspaceRoot(const QString &absoluteRootPath);

    void notifyCurrentFile(const QString &absoluteFilePath);
    void notifyLocalFileSaved(const QString &absoluteFilePath);
    void notifyLocalFileSynced(const QString &absoluteFilePath, qint64 version);
    void setAuthSession(const QString &token, const QString &username);
    void notifyLocalTextEdited(const QString &absoluteFilePath, const QString &text);
    void notifyLocalCursorMoved(const QString &absoluteFilePath, int line, int column);

    void clearAuthSession();
    QString authStatusLine() const;
    bool isSignedIn() const;

    QString collaborationProjectKey() const;
    QString authBearerToken() const;
    QString collaborationServerBaseUrl() const;

signals:
    void serverHealthCheckRequested(const QUrl &serverBaseUrl);
    void collaborationRosterSynced();
    void loginRequested();
    void authSessionChanged();
    void remoteFileUpdated(const QString &absoluteFilePath, const QString &text);
    void remoteCursorMoved(const QString &absoluteFilePath, const QString &username, int line, int column);

private:
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    void updateCollaborationChannelUi();
    void onCollaborationChannelButtonClicked();
    void sendPresenceJoin();
    void sendHeartbeat();
    void clearPresenceUi();
    void applyRosterJson(const QJsonObject &obj);
    void applyUserJoined(const QJsonObject &obj);
    void applyUserLeft(const QJsonObject &obj);
    void applyCurrentFileChanged(const QJsonObject &obj);
    void rebuildOnlineList();
    void appendActivityLine(const QString &line);
    QString relativeWorkspacePath(const QString &absolutePath) const;
    QString displayNameForClient(const QString &clientId) const;
    static bool hasStringContent(const QJsonObject &obj);

private slots:
    void onWebSocketMessage(const QString &text);
#endif

private:
    ServerEndpointSettings endpointSettings_;
    QString collaborationProjectId_{QStringLiteral("default")};
    QString workspaceRootAbsolute_;

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    QString localClientId_;
    QString authToken_;
    QString username_;
    QHash<QString, CollaboratorPeer> remotePeers_;
    QTimer heartbeatTimer_;
#endif

    void refreshAccountUi();

    QLabel *serverConnectionStatusLabel_ = nullptr;
    QLineEdit *serverBaseUrlEdit_ = nullptr;
    QPushButton *checkServerConnectionButton_ = nullptr;
    QLabel *collaborationChannelStatusLabel_ = nullptr;
    QPushButton *collaborationChannelButton_ = nullptr;
    QLabel *onlineMembersCaption_ = nullptr;
    QListWidget *onlineMembersList_ = nullptr;
    QLabel *activityCaption_ = nullptr;
    QListWidget *activityList_ = nullptr;
    QLabel *accountStatusLabel_ = nullptr;
    QPushButton *accountActionButton_ = nullptr;
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    CollaborationWebSocketClient *collaborationWsClient_ = nullptr;
#endif
};
