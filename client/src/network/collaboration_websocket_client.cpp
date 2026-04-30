#include "network/collaboration_websocket_client.h"

#include <QAbstractSocket>
#include <QUrlQuery>
#include <QWebSocket>

CollaborationWebSocketClient::CollaborationWebSocketClient(QObject *parent)
    : QObject(parent)
{
}

CollaborationWebSocketClient::~CollaborationWebSocketClient()
{
    disconnectFromServer();
}

QUrl CollaborationWebSocketClient::buildCollaborationWebSocketUrl(const QUrl &httpBase,
                                                                    const QString &projectId,
                                                                    const QString &accessToken)
{
    if (httpBase.scheme().isEmpty() || httpBase.host().isEmpty()) {
        return {};
    }

    QUrl u = httpBase;
    const QString scheme = u.scheme().toLower();
    if (scheme == QStringLiteral("https")) {
        u.setScheme(QStringLiteral("wss"));
    } else if (scheme == QStringLiteral("http")) {
        u.setScheme(QStringLiteral("ws"));
    } else {
        return {};
    }

    const QByteArray encodedId = QUrl::toPercentEncoding(projectId);
    u.setPath(QStringLiteral("/ws/projects/%1").arg(QString::fromUtf8(encodedId)));
    u.setQuery(QUrlQuery());

    if (!accessToken.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("token"), accessToken);
        u.setQuery(q);
    }
    return u;
}

void CollaborationWebSocketClient::ensureSocket()
{
    if (socket_) {
        return;
    }
    socket_ = std::make_unique<QWebSocket>(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(socket_.get(), &QWebSocket::connected, this, &CollaborationWebSocketClient::connected);
    connect(socket_.get(), &QWebSocket::disconnected, this, &CollaborationWebSocketClient::disconnected);
    connect(socket_.get(), &QWebSocket::textMessageReceived, this, &CollaborationWebSocketClient::textMessageReceived);
    connect(socket_.get(), &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(socket_->errorString());
    });
}

void CollaborationWebSocketClient::connectToServer(const QUrl &wsUrl)
{
    ensureSocket();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }
    socket_->open(wsUrl);
}

void CollaborationWebSocketClient::disconnectFromServer()
{
    if (socket_) {
        socket_->close();
        socket_.reset();
    }
}

void CollaborationWebSocketClient::sendTextMessage(const QString &message)
{
    if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->sendTextMessage(message);
    }
}

bool CollaborationWebSocketClient::isConnected() const
{
    return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}
