#pragma once

#include <QString>
#include <memory>

class QSettings;

class ServerEndpointSettings final {
public:
    explicit ServerEndpointSettings(const QString &iniFilePath = {});
    ~ServerEndpointSettings();

    ServerEndpointSettings(const ServerEndpointSettings &) = delete;
    ServerEndpointSettings &operator=(const ServerEndpointSettings &) = delete;

    static QString defaultServerBaseUrl();

    QString serverBaseUrl() const;
    void setServerBaseUrl(const QString &url);

private:
    static QString normalizeBaseUrl(QString url);

    std::unique_ptr<QSettings> settings_;
};
