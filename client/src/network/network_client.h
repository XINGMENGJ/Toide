#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class NetworkClient final : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    void checkHealth(const QUrl &serverBaseUrl);

signals:
    void healthChecked(bool online, const QString &message);

private:
    QNetworkAccessManager network_;
};
