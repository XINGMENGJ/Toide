#pragma once

#include <QJsonArray>
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

    void listWorkspaces(const QUrl &serverBaseUrl, const QString &bearerToken);
    void createWorkspace(const QUrl &serverBaseUrl, const QString &name, const QString &bearerToken);
    void fetchWorkspaceManifest(const QUrl &serverBaseUrl, const QString &projectKey, const QString &bearerToken);
    void fetchWorkspaceLatestFile(const QUrl &serverBaseUrl,
                                  const QString &projectKey,
                                  const QString &relativePath,
                                  const QString &bearerToken);

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
    void workspacesListFetched(bool ok, const QString &message, const QJsonArray &workspaces);
    void workspaceCreated(bool ok, const QString &message, const QString &id, const QString &name);
    void workspaceManifestFetched(bool ok, const QString &message, const QJsonArray &files);
    void workspaceLatestFileFetched(bool ok,
                                    const QString &message,
                                    const QString &relativePath,
                                    const QString &content,
                                    qint64 version);
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
