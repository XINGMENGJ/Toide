#include <QtTest/QtTest>

#include "network/collaboration_websocket_client.h"

#include <QHostAddress>
#include <QSignalSpy>
#include <QUrlQuery>
#include <QWebSocket>
#include <QWebSocketServer>

class CollaborationWebSocketClientTest final : public QObject {
    Q_OBJECT

private slots:
    void buildUrlMapsHttpToWsWithPath();
    void buildUrlIncludesClientIdQuery();
    void echoRoundTripOverLocalServer();
};

void CollaborationWebSocketClientTest::buildUrlMapsHttpToWsWithPath()
{
    const QUrl http(QStringLiteral("http://127.0.0.1:8848"));
    const QUrl ws = CollaborationWebSocketClient::buildCollaborationWebSocketUrl(http, QStringLiteral("proj-a"));
    QCOMPARE(ws.scheme(), QStringLiteral("ws"));
    QCOMPARE(ws.host(), QStringLiteral("127.0.0.1"));
    QCOMPARE(ws.port(), 8848);
    QCOMPARE(ws.path(), QStringLiteral("/ws/projects/proj-a"));
    QVERIFY(ws.query().isEmpty());

    const QUrl httpToken =
        CollaborationWebSocketClient::buildCollaborationWebSocketUrl(http, QStringLiteral("p"), QStringLiteral("tok/1"));
    const QUrlQuery q(httpToken);
    QCOMPARE(q.queryItemValue(QStringLiteral("token")), QStringLiteral("tok/1"));
}

void CollaborationWebSocketClientTest::buildUrlIncludesClientIdQuery()
{
    const QUrl http(QStringLiteral("http://127.0.0.1:8848"));
    const QUrl ws =
        CollaborationWebSocketClient::buildCollaborationWebSocketUrl(http, QStringLiteral("proj-a"), QString(), QStringLiteral("cid-1"));
    QUrlQuery qc(ws);
    QCOMPARE(qc.queryItemValue(QStringLiteral("clientId")), QStringLiteral("cid-1"));
    QVERIFY(qc.queryItemValue(QStringLiteral("token")).isEmpty());

    const QUrl both = CollaborationWebSocketClient::buildCollaborationWebSocketUrl(
        http, QStringLiteral("p"), QStringLiteral("tok"), QStringLiteral("cid-2"));
    QUrlQuery qb(both);
    QCOMPARE(qb.queryItemValue(QStringLiteral("token")), QStringLiteral("tok"));
    QCOMPARE(qb.queryItemValue(QStringLiteral("clientId")), QStringLiteral("cid-2"));
}

void CollaborationWebSocketClientTest::echoRoundTripOverLocalServer()
{
    QWebSocketServer server(QStringLiteral("toide-test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QObject::connect(&server, &QWebSocketServer::newConnection, &server, [&]() {
        auto *peer = server.nextPendingConnection();
        QObject::connect(peer, &QWebSocket::textMessageReceived, peer, [peer](const QString &text) {
            peer->sendTextMessage(QLatin1String("echo:") + text);
        });
    });

    CollaborationWebSocketClient client;
    QSignalSpy connectedSpy(&client, &CollaborationWebSocketClient::connected);
    client.connectToServer(server.serverUrl());
    QVERIFY2(connectedSpy.wait(3000), "WebSocket client did not connect");

    QSignalSpy replySpy(&client, &CollaborationWebSocketClient::textMessageReceived);
    client.sendTextMessage(QStringLiteral("hi"));
    QVERIFY2(replySpy.wait(3000), "no echo reply");
    QCOMPARE(replySpy.first().first().toString(), QStringLiteral("echo:hi"));

    client.disconnectFromServer();
}

QTEST_MAIN(CollaborationWebSocketClientTest)

#include "collaboration_websocket_client_test.moc"
