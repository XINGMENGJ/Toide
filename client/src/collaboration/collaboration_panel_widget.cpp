#include "collaboration/collaboration_panel_widget.h"

#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
#include "network/collaboration_websocket_client.h"
#endif

CollaborationPanelWidget::CollaborationPanelWidget(QWidget *parent, const QString &endpointSettingsIniPath)
    : QWidget(parent)
    , endpointSettings_(endpointSettingsIniPath)
    , serverConnectionStatusLabel_(new QLabel(QStringLiteral("Server: Not connected"), this))
    , serverBaseUrlEdit_(new QLineEdit(this))
    , checkServerConnectionButton_(new QPushButton(QStringLiteral("Check server"), this))
    , collaborationChannelStatusLabel_(new QLabel(this))
    , collaborationChannelButton_(new QPushButton(QStringLiteral("Connect collaboration channel"), this))
{
    serverConnectionStatusLabel_->setObjectName(QStringLiteral("serverConnectionStatusLabel"));
    serverBaseUrlEdit_->setObjectName(QStringLiteral("serverBaseUrlLineEdit"));
    checkServerConnectionButton_->setObjectName(QStringLiteral("checkServerConnectionButton"));
    collaborationChannelStatusLabel_->setObjectName(QStringLiteral("collaborationChannelStatusLabel"));
    collaborationChannelButton_->setObjectName(QStringLiteral("collaborationChannelButton"));

    serverBaseUrlEdit_->setPlaceholderText(ServerEndpointSettings::defaultServerBaseUrl());
    serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    collaborationWsClient_ = new CollaborationWebSocketClient(this);
    collaborationChannelStatusLabel_->setText(QStringLiteral("Collaboration channel: disconnected"));
    collaborationChannelButton_->setEnabled(true);

    connect(collaborationWsClient_, &CollaborationWebSocketClient::connected, this, [this]() {
        updateCollaborationChannelUi();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::disconnected, this, [this]() {
        updateCollaborationChannelUi();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::errorOccurred, this, [this](const QString &message) {
        collaborationChannelStatusLabel_->setText(QStringLiteral("Collaboration channel: error (%1)").arg(message));
        collaborationChannelButton_->setText(QStringLiteral("Connect collaboration channel"));
    });
    connect(collaborationChannelButton_, &QPushButton::clicked, this,
            &CollaborationPanelWidget::onCollaborationChannelButtonClicked);
#else
    collaborationChannelStatusLabel_->setText(
        QStringLiteral("Collaboration channel: unavailable (Qt WebSockets not in this build)"));
    collaborationChannelButton_->setVisible(false);
#endif

    auto *titleLabel = new QLabel(QStringLiteral("Collaboration"), this);
    titleLabel->setObjectName(QStringLiteral("collaborationPanelTitleLabel"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(titleLabel);
    layout->addWidget(serverBaseUrlEdit_);
    layout->addWidget(serverConnectionStatusLabel_);
    layout->addWidget(checkServerConnectionButton_);
    layout->addWidget(collaborationChannelStatusLabel_);
    layout->addWidget(collaborationChannelButton_);
    layout->addStretch(1);

    connect(serverBaseUrlEdit_, &QLineEdit::editingFinished, this, [this]() {
        endpointSettings_.setServerBaseUrl(serverBaseUrlEdit_->text());
        serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
        if (collaborationWsClient_->isConnected()) {
            collaborationWsClient_->disconnectFromServer();
            updateCollaborationChannelUi();
        }
#endif
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

void CollaborationPanelWidget::setWorkspaceKey(const QString &workspacePath)
{
    if (workspacePath.isEmpty()) {
        collaborationProjectId_ = QStringLiteral("default");
        return;
    }
    const QString name = QFileInfo(workspacePath).fileName();
    collaborationProjectId_ = name.isEmpty() ? QStringLiteral("default") : name;
}

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
void CollaborationPanelWidget::updateCollaborationChannelUi()
{
    const bool connected = collaborationWsClient_->isConnected();
    collaborationChannelButton_->setText(
        connected ? QStringLiteral("Disconnect collaboration channel") : QStringLiteral("Connect collaboration channel"));
    collaborationChannelStatusLabel_->setText(connected ? QStringLiteral("Collaboration channel: connected")
                                                         : QStringLiteral("Collaboration channel: disconnected"));
}

void CollaborationPanelWidget::onCollaborationChannelButtonClicked()
{
    if (collaborationWsClient_->isConnected()) {
        collaborationWsClient_->disconnectFromServer();
        updateCollaborationChannelUi();
        return;
    }

    endpointSettings_.setServerBaseUrl(serverBaseUrlEdit_->text());
    serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());

    const QUrl http(endpointSettings_.serverBaseUrl());
    const QUrl ws =
        CollaborationWebSocketClient::buildCollaborationWebSocketUrl(http, collaborationProjectId_, QString());

    if (ws.scheme().isEmpty() || ws.host().isEmpty()) {
        collaborationChannelStatusLabel_->setText(
            QStringLiteral("Collaboration channel: invalid server URL for WebSocket"));
        return;
    }

    collaborationChannelStatusLabel_->setText(QStringLiteral("Collaboration channel: connecting..."));
    collaborationWsClient_->connectToServer(ws);
}
#endif
