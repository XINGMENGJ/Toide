#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <memory>

class QWebSocket;

class CollaborationWebSocketClient final : public QObject {
    Q_OBJECT

public:
    explicit CollaborationWebSocketClient(QObject *parent = nullptr);
    ~CollaborationWebSocketClient() override;

    CollaborationWebSocketClient(const CollaborationWebSocketClient &) = delete;
    CollaborationWebSocketClient &operator=(const CollaborationWebSocketClient &) = delete;

    static QUrl buildCollaborationWebSocketUrl(const QUrl &httpBase,
                                               const QString &projectId,
                                               const QString &accessToken = {},
                                               const QString &clientId = {});

    void connectToServer(const QUrl &wsUrl);
    void disconnectFromServer();
    void sendTextMessage(const QString &message);

    bool isConnected() const;
    bool isReconnectEnabled() const;
    int reconnectIntervalMs() const;

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString &message);
    void errorOccurred(const QString &message);

private:
    void ensureSocket();
    void scheduleReconnect();
    void openLastUrl();

    std::unique_ptr<QWebSocket> socket_;
    QTimer reconnectTimer_;
    QUrl lastUrl_;
    bool reconnectEnabled_ = false;
    bool userDisconnectRequested_ = false;
};
