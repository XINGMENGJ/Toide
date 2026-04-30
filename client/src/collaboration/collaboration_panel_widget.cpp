#include "collaboration/collaboration_panel_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QUrl>

CollaborationPanelWidget::CollaborationPanelWidget(QWidget *parent, const QString &endpointSettingsIniPath)
    : QWidget(parent)
    , endpointSettings_(endpointSettingsIniPath)
    , serverConnectionStatusLabel_(new QLabel(QStringLiteral("Server: Not connected"), this))
    , serverBaseUrlEdit_(new QLineEdit(this))
    , checkServerConnectionButton_(new QPushButton(QStringLiteral("Check server"), this))
{
    serverConnectionStatusLabel_->setObjectName(QStringLiteral("serverConnectionStatusLabel"));
    serverBaseUrlEdit_->setObjectName(QStringLiteral("serverBaseUrlLineEdit"));
    checkServerConnectionButton_->setObjectName(QStringLiteral("checkServerConnectionButton"));

    serverBaseUrlEdit_->setPlaceholderText(ServerEndpointSettings::defaultServerBaseUrl());
    serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());

    auto *titleLabel = new QLabel(QStringLiteral("Collaboration"), this);
    titleLabel->setObjectName(QStringLiteral("collaborationPanelTitleLabel"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(titleLabel);
    layout->addWidget(serverBaseUrlEdit_);
    layout->addWidget(serverConnectionStatusLabel_);
    layout->addWidget(checkServerConnectionButton_);
    layout->addStretch(1);

    connect(serverBaseUrlEdit_, &QLineEdit::editingFinished, this, [this]() {
        endpointSettings_.setServerBaseUrl(serverBaseUrlEdit_->text());
        serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());
    });

    connect(checkServerConnectionButton_, &QPushButton::clicked, this, [this]() {
        endpointSettings_.setServerBaseUrl(serverBaseUrlEdit_->text());
        serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());
        serverConnectionStatusLabel_->setText(QStringLiteral("Server: Checking..."));
        emit serverHealthCheckRequested(QUrl(endpointSettings_.serverBaseUrl()));
    });
}

void CollaborationPanelWidget::setServerStatus(bool online, const QString &message)
{
    serverConnectionStatusLabel_->setText(QStringLiteral("Server: %1").arg(online ? QStringLiteral("Online") : message));
}
