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
