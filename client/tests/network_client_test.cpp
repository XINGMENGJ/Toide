#include <QtTest/QtTest>

#include "network/network_client.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>

class NetworkClientTest final : public QObject {
    Q_OBJECT

private slots:
    void healthCheckReportsOnlineWhenServerReturnsOk();
    void healthCheckReportsOfflineWhenServerIsUnavailable();
};

static QUrl localServerUrl(const QTcpServer &server)
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
}

void NetworkClientTest::healthCheckReportsOnlineWhenServerReturnsOk()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        auto *socket = server.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: 15\r\n\r\n{\"status\":\"ok\"}");
            socket->disconnectFromHost();
        });
    });

    NetworkClient client;
    QSignalSpy spy(&client, &NetworkClient::healthChecked);
    QVERIFY(spy.isValid());

    client.checkHealth(localServerUrl(server));

    QVERIFY(spy.wait(3000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    QVERIFY(spy.at(0).at(1).toString().contains(QStringLiteral("Online")));
}

void NetworkClientTest::healthCheckReportsOfflineWhenServerIsUnavailable()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        auto *socket = server.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            socket->write("HTTP/1.1 500 Internal Server Error\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: 18\r\n\r\n{\"status\":\"error\"}");
            socket->disconnectFromHost();
        });
    });

    NetworkClient client;
    QSignalSpy spy(&client, &NetworkClient::healthChecked);
    QVERIFY(spy.isValid());

    client.checkHealth(localServerUrl(server));

    QVERIFY(spy.wait(3000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QVERIFY(spy.at(0).at(1).toString().contains(QStringLiteral("Offline")));
}

QTEST_MAIN(NetworkClientTest)

#include "network_client_test.moc"
