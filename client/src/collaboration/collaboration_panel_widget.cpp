#include "collaboration/collaboration_panel_widget.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CollaborationPanelWidget::CollaborationPanelWidget(QWidget *parent)
    : QWidget(parent)
    , serverConnectionStatusLabel_(new QLabel(QStringLiteral("Server: Not connected"), this))
    , checkServerConnectionButton_(new QPushButton(QStringLiteral("Check server"), this))
{
    serverConnectionStatusLabel_->setObjectName(QStringLiteral("serverConnectionStatusLabel"));
    checkServerConnectionButton_->setObjectName(QStringLiteral("checkServerConnectionButton"));

    auto *titleLabel = new QLabel(QStringLiteral("Collaboration"), this);
    titleLabel->setObjectName(QStringLiteral("collaborationPanelTitleLabel"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(titleLabel);
    layout->addWidget(serverConnectionStatusLabel_);
    layout->addWidget(checkServerConnectionButton_);
    layout->addStretch(1);

    connect(checkServerConnectionButton_, &QPushButton::clicked, this, [this]() {
        serverConnectionStatusLabel_->setText(QStringLiteral("Server: Checking..."));
        emit serverHealthCheckRequested();
    });
}

void CollaborationPanelWidget::setServerStatus(bool online, const QString &message)
{
    serverConnectionStatusLabel_->setText(QStringLiteral("Server: %1").arg(online ? QStringLiteral("Online") : message));
}
