#pragma once

#include <QObject>
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
                                               const QString &accessToken = {});

    void connectToServer(const QUrl &wsUrl);
    void disconnectFromServer();
    void sendTextMessage(const QString &message);

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString &message);
    void errorOccurred(const QString &message);

private:
    void ensureSocket();

    std::unique_ptr<QWebSocket> socket_;
};
