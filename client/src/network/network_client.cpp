#include "network/network_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopeGuard>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
{
}

void NetworkClient::checkHealth(const QUrl &serverBaseUrl)
{
    auto healthUrl = serverBaseUrl;
    healthUrl.setPath(QStringLiteral("/api/health"));

    auto *reply = network_.get(QNetworkRequest(healthUrl));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const auto guard = qScopeGuard([reply]() {
            reply->deleteLater();
        });

        if (reply->error() != QNetworkReply::NoError) {
            emit healthChecked(false, QStringLiteral("Offline: %1").arg(reply->errorString()));
            return;
        }

        const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto payload = QJsonDocument::fromJson(reply->readAll()).object();
        const auto isOnline = statusCode == 200 && payload.value(QStringLiteral("status")).toString() == QStringLiteral("ok");

        emit healthChecked(isOnline, isOnline ? QStringLiteral("Online") : QStringLiteral("Offline: unhealthy server response"));
    });
}

void NetworkClient::login(const QUrl &serverBaseUrl, const QString &username, const QString &password)
{
    submitAuth(serverBaseUrl, QStringLiteral("/api/auth/login"), username, password);
}

void NetworkClient::registerUser(const QUrl &serverBaseUrl, const QString &username, const QString &password)
{
    submitAuth(serverBaseUrl, QStringLiteral("/api/auth/register"), username, password);
}

void NetworkClient::submitAuth(const QUrl &serverBaseUrl, const QString &path, const QString &username, const QString &password)
{
    auto authUrl = serverBaseUrl;
    authUrl.setPath(path);

    QJsonObject payload;
    payload[QStringLiteral("username")] = username;
    payload[QStringLiteral("password")] = password;

    QNetworkRequest request(authUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto *reply = network_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const auto guard = qScopeGuard([reply]() {
            reply->deleteLater();
        });

        const auto body = QJsonDocument::fromJson(reply->readAll()).object();
        const QString message = body.value(QStringLiteral("message")).toString(reply->errorString());
        if (reply->error() != QNetworkReply::NoError) {
            emit authFinished(false, message.isEmpty() ? reply->errorString() : message, {}, {});
            return;
        }

        const bool success = body.value(QStringLiteral("success")).toBool(false);
        const QString token = body.value(QStringLiteral("token")).toString();
        const QString username = body.value(QStringLiteral("user")).toObject().value(QStringLiteral("username")).toString();
        emit authFinished(success && !token.isEmpty(),
                          message.isEmpty() ? (success ? QStringLiteral("Authenticated") : QStringLiteral("Authentication failed")) : message,
                          token,
                          username);
    });
}
