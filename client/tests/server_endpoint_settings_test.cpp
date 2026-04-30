#include <QtTest/QtTest>

#include "settings/server_endpoint_settings.h"

#include <QTemporaryDir>

class ServerEndpointSettingsTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultUrlIs8848Local();
    void roundTripPersistsNormalizedUrl();
    void invalidInputFallsBackToDefault();
    void ensureCollaborationClientIdCreatesAndPersists();
};

void ServerEndpointSettingsTest::defaultUrlIs8848Local()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    ServerEndpointSettings settings(dir.filePath(QStringLiteral("e.ini")));
    QCOMPARE(settings.serverBaseUrl(), ServerEndpointSettings::defaultServerBaseUrl());
}

void ServerEndpointSettingsTest::roundTripPersistsNormalizedUrl()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("s.ini"));

    {
        ServerEndpointSettings settings(path);
        settings.setServerBaseUrl(QStringLiteral("http://example.com:9000/ignored/path"));
    }

    ServerEndpointSettings loaded(path);
    QCOMPARE(loaded.serverBaseUrl(), QStringLiteral("http://example.com:9000"));
}

void ServerEndpointSettingsTest::invalidInputFallsBackToDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    ServerEndpointSettings settings(dir.filePath(QStringLiteral("bad.ini")));
    settings.setServerBaseUrl(QStringLiteral("not a url at all"));
    QCOMPARE(settings.serverBaseUrl(), ServerEndpointSettings::defaultServerBaseUrl());
}

void ServerEndpointSettingsTest::ensureCollaborationClientIdCreatesAndPersists()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("clientid.ini"));
    QString first;
    {
        ServerEndpointSettings s(path);
        QVERIFY(s.collaborationClientId().isEmpty());
        first = s.ensureCollaborationClientId();
        QVERIFY(!first.isEmpty());
        QCOMPARE(s.collaborationClientId(), first);
    }
    ServerEndpointSettings loaded(path);
    QCOMPARE(loaded.collaborationClientId(), first);
    QCOMPARE(loaded.ensureCollaborationClientId(), first);
}

QTEST_MAIN(ServerEndpointSettingsTest)

#include "server_endpoint_settings_test.moc"
