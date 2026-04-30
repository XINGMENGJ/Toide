#include "network/network_client.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopeGuard>

#include <QUrlQuery>

namespace {

QString errorMessageFromBody(const QJsonObject &body, const QString &fallback)
{
    const QJsonObject error = body.value(QStringLiteral("error")).toObject();
    const QString message = error.value(QStringLiteral("message")).toString();
    if (!message.isEmpty()) {
        return message;
    }
    const QString topMessage = body.value(QStringLiteral("message")).toString();
    return topMessage.isEmpty() ? fallback : topMessage;
}

} // namespace

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
            emit healthChecked(false, QStringLiteral("离线：%1").arg(reply->errorString()));
            return;
        }

        const auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto payload = QJsonDocument::fromJson(reply->readAll()).object();
        if (statusCode < 200 || statusCode >= 300) {
            const QString hint = statusCode == 404
                ? QStringLiteral(" — 未找到 /api/health，请确认是否为 Toide 服务端")
                : QString();
            emit healthChecked(false,
                QStringLiteral("HTTP %1%2").arg(QString::number(statusCode), hint));
            return;
        }
        const auto isOnline = payload.value(QStringLiteral("status")).toString() == QStringLiteral("ok");

        emit healthChecked(isOnline, isOnline ? QStringLiteral("在线") : QStringLiteral("离线：服务端响应异常"));
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

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode < 200 || statusCode >= 300) {
            const QString hint = statusCode == 404
                ? QStringLiteral(" — 未找到 /api/auth，请先启动 Toide 服务端")
                : QString();
            emit authFinished(
                false,
                QStringLiteral("HTTP %1%2").arg(QString::number(statusCode), hint),
                {},
                {});
            return;
        }

        const bool success = body.value(QStringLiteral("success")).toBool(false);
        const QString token = body.value(QStringLiteral("token")).toString();
        const QString username = body.value(QStringLiteral("user")).toObject().value(QStringLiteral("username")).toString();
        emit authFinished(success && !token.isEmpty(),
                          message.isEmpty() ? (success ? QStringLiteral("认证成功") : QStringLiteral("认证失败")) : message,
                          token,
                          username);
    });
}

void NetworkClient::fetchWorkspaceFileVersion(const QUrl &serverBaseUrl,
                                              const QString &projectKey,
                                              const QString &relativePath,
                                              const QString &absoluteFilePath,
                                              const QString &bearerToken)
{
    QUrl url(serverBaseUrl);
    url.setPath(QStringLiteral("/api/workspace/files/version"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("projectKey"), projectKey);
    query.addQueryItem(QStringLiteral("path"), relativePath);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QByteArray authHeader = QStringLiteral("Bearer %1").arg(bearerToken).toUtf8();
    request.setRawHeader("Authorization", authHeader);

    auto *reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, absoluteFilePath]() {
        const auto guard = qScopeGuard([reply]() {
            reply->deleteLater();
        });
        if (reply->error() != QNetworkReply::NoError) {
            emit workspaceFileVersionFetched(false, reply->errorString(), absoluteFilePath, -1);
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto body = QJsonDocument::fromJson(reply->readAll()).object();
        if (status == 401) {
            emit workspaceFileVersionFetched(false,
                                             body.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString(
                                                 QStringLiteral("未授权")),
                                             absoluteFilePath,
                                             -1);
            return;
        }
        if (status < 200 || status >= 300) {
            emit workspaceFileVersionFetched(false,
                                             QStringLiteral("HTTP %1").arg(QString::number(status)),
                                             absoluteFilePath,
                                             -1);
            return;
        }
        const qint64 v = body.value(QStringLiteral("version")).toVariant().toLongLong();
        emit workspaceFileVersionFetched(true, QString(), absoluteFilePath, v);
    });
}

void NetworkClient::putWorkspaceFileContent(const QUrl &serverBaseUrl,
                                            const QString &projectKey,
                                            const QString &relativePath,
                                            const QString &absoluteFilePath,
                                            qint64 baseVersion,
                                            const QString &content,
                                            const QString &bearerToken)
{
    QUrl url(serverBaseUrl);
    url.setPath(QStringLiteral("/api/workspace/files/content"));

    QJsonObject payload;
    payload[QStringLiteral("projectKey")] = projectKey;
    payload[QStringLiteral("filePath")] = relativePath;
    payload[QStringLiteral("baseVersion")] = static_cast<double>(baseVersion);
    payload[QStringLiteral("content")] = content;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(bearerToken).toUtf8());

    auto *reply = network_.put(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, absoluteFilePath]() {
        const auto guard = qScopeGuard([reply]() {
            reply->deleteLater();
        });
        const QByteArray rawBody = reply->readAll();
        const auto body = QJsonDocument::fromJson(rawBody).object();
        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 409) {
                const auto err = body.value(QStringLiteral("error")).toObject();
                const qint64 latest = err.value(QStringLiteral("latestVersion")).toVariant().toLongLong();
                emit workspaceFileUploadFinished(false,
                                                 err.value(QStringLiteral("message")).toString(QStringLiteral("版本冲突")),
                                                 absoluteFilePath,
                                                 -1,
                                                 true,
                                                 latest);
                return;
            }
            emit workspaceFileUploadFinished(false,
                                             errorMessageFromBody(body, reply->errorString()),
                                             absoluteFilePath,
                                             -1,
                                             false,
                                             -1);
            return;
        }
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 409) {
            const auto err = body.value(QStringLiteral("error")).toObject();
            const qint64 latest = err.value(QStringLiteral("latestVersion")).toVariant().toLongLong();
            emit workspaceFileUploadFinished(false,
                                             err.value(QStringLiteral("message")).toString(QStringLiteral("版本冲突")),
                                             absoluteFilePath,
                                             -1,
                                             true,
                                             latest);
            return;
        }
        if (status < 200 || status >= 300) {
            emit workspaceFileUploadFinished(false,
                                             errorMessageFromBody(body, QStringLiteral("HTTP %1").arg(QString::number(status))),
                                             absoluteFilePath,
                                             -1,
                                             false,
                                             -1);
            return;
        }
        const qint64 newV = body.value(QStringLiteral("version")).toVariant().toLongLong();
        emit workspaceFileUploadFinished(true, QString(), absoluteFilePath, newV, false, -1);
    });
}
