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

    void fetchWorkspaceFileVersion(const QUrl &serverBaseUrl,
                                   const QString &projectKey,
                                   const QString &relativePath,
                                   const QString &absoluteFilePath,
                                   const QString &bearerToken);
    void putWorkspaceFileContent(const QUrl &serverBaseUrl,
                                 const QString &projectKey,
                                 const QString &relativePath,
                                 const QString &absoluteFilePath,
                                 qint64 baseVersion,
                                 const QString &content,
                                 const QString &bearerToken);

signals:
    void healthChecked(bool online, const QString &message);
    void authFinished(bool ok, const QString &message, const QString &token, const QString &username);
    void workspaceFileVersionFetched(bool ok,
                                     const QString &message,
                                     const QString &absoluteFilePath,
                                     qint64 version);
    void workspaceFileUploadFinished(bool ok,
                                       const QString &message,
                                       const QString &absoluteFilePath,
                                       qint64 newVersion,
                                       bool conflict,
                                       qint64 serverLatestVersion);

private:
    void submitAuth(const QUrl &serverBaseUrl, const QString &path, const QString &username, const QString &password);

    QNetworkAccessManager network_;
};
