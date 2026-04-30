#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class NetworkClient final : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    void checkHealth(const QUrl &serverBaseUrl);
    void login(const QUrl &serverBaseUrl, const QString &username, const QString &password);
    void registerUser(const QUrl &serverBaseUrl, const QString &username, const QString &password);

signals:
    void healthChecked(bool online, const QString &message);
    void authFinished(bool ok, const QString &message, const QString &token, const QString &username);

private:
    void submitAuth(const QUrl &serverBaseUrl, const QString &path, const QString &username, const QString &password);

    QNetworkAccessManager network_;
};
