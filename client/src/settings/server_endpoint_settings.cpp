#include "settings/server_endpoint_settings.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>

ServerEndpointSettings::ServerEndpointSettings(const QString &iniFilePath)
{
    QString path = iniFilePath;
    if (path.isEmpty()) {
        const auto root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(root);
        path = root + QStringLiteral("/toide-client.ini");
    }
    settings_ = std::make_unique<QSettings>(path, QSettings::IniFormat);
}

ServerEndpointSettings::~ServerEndpointSettings() = default;

QString ServerEndpointSettings::defaultServerBaseUrl()
{
    return QStringLiteral("http://127.0.0.1:8848");
}

QString ServerEndpointSettings::normalizeBaseUrl(QString url)
{
    url = url.trimmed();
    if (url.isEmpty()) {
        return defaultServerBaseUrl();
    }

    QUrl parsed = QUrl::fromUserInput(url);
    if (!parsed.isValid() || parsed.scheme().isEmpty()) {
        return defaultServerBaseUrl();
    }

    const QString host = parsed.host();
    if (host.isEmpty()) {
        return defaultServerBaseUrl();
    }

    const int port = parsed.port();
    if (port <= 0) {
        return QStringLiteral("%1://%2").arg(parsed.scheme(), host);
    }
    return QStringLiteral("%1://%2:%3").arg(parsed.scheme(), host).arg(port);
}

QString ServerEndpointSettings::serverBaseUrl() const
{
    const auto v = settings_->value(QStringLiteral("network/serverBaseUrl"), defaultServerBaseUrl()).toString();
    return normalizeBaseUrl(v);
}

void ServerEndpointSettings::setServerBaseUrl(const QString &url)
{
    settings_->setValue(QStringLiteral("network/serverBaseUrl"), normalizeBaseUrl(url));
    settings_->sync();
}

QString ServerEndpointSettings::collaborationClientId() const
{
    return settings_->value(QStringLiteral("collaboration/clientId")).toString().trimmed();
}

QString ServerEndpointSettings::ensureCollaborationClientId()
{
    const QString existing = collaborationClientId();
    if (!existing.isEmpty()) {
        return existing;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    settings_->setValue(QStringLiteral("collaboration/clientId"), id);
    settings_->sync();
    return id;
}
