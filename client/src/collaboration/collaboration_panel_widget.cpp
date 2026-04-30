#include "collaboration/collaboration_panel_widget.h"

#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
#include "network/collaboration_websocket_client.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#endif

CollaborationPanelWidget::CollaborationPanelWidget(QWidget *parent, const QString &endpointSettingsIniPath)
    : QWidget(parent)
    , endpointSettings_(endpointSettingsIniPath)
    , serverConnectionStatusLabel_(new QLabel(QStringLiteral("Server: Not connected"), this))
    , serverBaseUrlEdit_(new QLineEdit(this))
    , checkServerConnectionButton_(new QPushButton(QStringLiteral("Check server"), this))
    , collaborationChannelStatusLabel_(new QLabel(this))
    , collaborationChannelButton_(new QPushButton(QStringLiteral("Connect collaboration channel"), this))
    , onlineMembersCaption_(new QLabel(QStringLiteral("Online in this project"), this))
    , onlineMembersList_(new QListWidget(this))
    , activityCaption_(new QLabel(QStringLiteral("Collaboration activity"), this))
    , activityList_(new QListWidget(this))
{
    serverConnectionStatusLabel_->setObjectName(QStringLiteral("serverConnectionStatusLabel"));
    serverBaseUrlEdit_->setObjectName(QStringLiteral("serverBaseUrlLineEdit"));
    checkServerConnectionButton_->setObjectName(QStringLiteral("checkServerConnectionButton"));
    collaborationChannelStatusLabel_->setObjectName(QStringLiteral("collaborationChannelStatusLabel"));
    collaborationChannelButton_->setObjectName(QStringLiteral("collaborationChannelButton"));
    onlineMembersCaption_->setObjectName(QStringLiteral("collaborationOnlineCaption"));
    onlineMembersList_->setObjectName(QStringLiteral("collaborationOnlineMembersList"));
    activityCaption_->setObjectName(QStringLiteral("collaborationActivityCaption"));
    activityList_->setObjectName(QStringLiteral("collaborationActivityList"));

    serverBaseUrlEdit_->setPlaceholderText(ServerEndpointSettings::defaultServerBaseUrl());
    serverBaseUrlEdit_->setText(endpointSettings_.serverBaseUrl());
    authToken_ = endpointSettings_.authToken();
    username_ = endpointSettings_.username();

    onlineMembersList_->setMinimumHeight(96);
    activityList_->setMinimumHeight(120);
    activityList_->setAlternatingRowColors(true);

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    collaborationWsClient_ = new CollaborationWebSocketClient(this);
    collaborationChannelStatusLabel_->setText(QStringLiteral("Collaboration channel: disconnected"));
    collaborationChannelButton_->setEnabled(true);

    connect(collaborationWsClient_, &CollaborationWebSocketClient::connected, this, [this]() {
        updateCollaborationChannelUi();
        sendPresenceJoin();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::disconnected, this, [this]() {
        updateCollaborationChannelUi();
        clearPresenceUi();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::textMessageReceived, this,
            &CollaborationPanelWidget::onWebSocketMessage);
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
    onlineMembersCaption_->setVisible(false);
    onlineMembersList_->setVisible(false);
    activityCaption_->setVisible(false);
    activityList_->setVisible(false);
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
    layout->addWidget(onlineMembersCaption_);
    layout->addWidget(onlineMembersList_, 2);
    layout->addWidget(activityCaption_);
    layout->addWidget(activityList_, 2);
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

void CollaborationPanelWidget::setWorkspaceRoot(const QString &absoluteRootPath)
{
    if (absoluteRootPath.isEmpty()) {
        workspaceRootAbsolute_.clear();
        return;
    }
    workspaceRootAbsolute_ = QFileInfo(absoluteRootPath).absoluteFilePath();
}

void CollaborationPanelWidget::notifyCurrentFile(const QString &absoluteFilePath)
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    Q_UNUSED(absoluteFilePath);
#else
    if (collaborationWsClient_ == nullptr || !collaborationWsClient_->isConnected()) {
        return;
    }
    if (localClientId_.isEmpty()) {
        return;
    }
    const QString rel = relativeWorkspacePath(absoluteFilePath);
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("presence.current_file");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    o[QStringLiteral("filePath")] = rel;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));

    CollaboratorPeer selfRow;
    selfRow.clientId = localClientId_;
    selfRow.currentFile = rel;
    remotePeers_.insert(localClientId_, selfRow);
    rebuildOnlineList();
#endif
}

void CollaborationPanelWidget::notifyLocalFileSaved(const QString &absoluteFilePath)
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    Q_UNUSED(absoluteFilePath);
#else
    if (collaborationWsClient_ == nullptr || !collaborationWsClient_->isConnected()) {
        return;
    }
    if (localClientId_.isEmpty()) {
        return;
    }
    const QString rel = relativeWorkspacePath(absoluteFilePath);
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("collab.file_saved");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    o[QStringLiteral("filePath")] = rel;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
#endif
}

void CollaborationPanelWidget::setAuthSession(const QString &token, const QString &username)
{
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    authToken_ = token;
    username_ = username;
    endpointSettings_.setAuthSession(token, username);
#else
    Q_UNUSED(token);
    Q_UNUSED(username);
#endif
}

void CollaborationPanelWidget::notifyLocalTextEdited(const QString &absoluteFilePath, const QString &text)
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    Q_UNUSED(absoluteFilePath);
    Q_UNUSED(text);
#else
    if (collaborationWsClient_ == nullptr || !collaborationWsClient_->isConnected() || localClientId_.isEmpty()) {
        return;
    }
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("editor.patch");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    o[QStringLiteral("filePath")] = relativeWorkspacePath(absoluteFilePath);
    o[QStringLiteral("content")] = text;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
#endif
}

void CollaborationPanelWidget::notifyLocalCursorMoved(const QString &absoluteFilePath, int line, int column)
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    Q_UNUSED(absoluteFilePath);
    Q_UNUSED(line);
    Q_UNUSED(column);
#else
    if (collaborationWsClient_ == nullptr || !collaborationWsClient_->isConnected() || localClientId_.isEmpty()) {
        return;
    }
    QJsonObject cursor;
    cursor[QStringLiteral("line")] = line;
    cursor[QStringLiteral("column")] = column;
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("editor.cursor");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    o[QStringLiteral("filePath")] = relativeWorkspacePath(absoluteFilePath);
    o[QStringLiteral("cursor")] = cursor;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
#endif
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
    localClientId_ = endpointSettings_.ensureCollaborationClientId();

    const QUrl http(endpointSettings_.serverBaseUrl());
    const QUrl ws = CollaborationWebSocketClient::buildCollaborationWebSocketUrl(
        http, collaborationProjectId_, authToken_, localClientId_);

    if (ws.scheme().isEmpty() || ws.host().isEmpty()) {
        collaborationChannelStatusLabel_->setText(
            QStringLiteral("Collaboration channel: invalid server URL for WebSocket"));
        return;
    }

    collaborationChannelStatusLabel_->setText(QStringLiteral("Collaboration channel: connecting..."));
    collaborationWsClient_->connectToServer(ws);
}

void CollaborationPanelWidget::sendPresenceJoin()
{
    if (localClientId_.isEmpty()) {
        localClientId_ = endpointSettings_.ensureCollaborationClientId();
    }
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("presence.join");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

void CollaborationPanelWidget::clearPresenceUi()
{
    remotePeers_.clear();
    onlineMembersList_->clear();
    appendActivityLine(QStringLiteral("Disconnected from collaboration channel."));
}

void CollaborationPanelWidget::rebuildOnlineList()
{
    onlineMembersList_->clear();
    for (auto it = remotePeers_.constBegin(); it != remotePeers_.constEnd(); ++it) {
        const QString &cid = it.key();
        const CollaboratorPeer &p = it.value();
        QString line = displayNameForClient(cid);
        if (cid == localClientId_) {
            line.append(QStringLiteral(" (you)"));
        }
        if (p.currentFile.isEmpty()) {
            line.append(QStringLiteral(" — "));
            line.append(QStringLiteral("(no file)"));
        } else {
            line.append(QStringLiteral(" — "));
            line.append(p.currentFile);
        }
        onlineMembersList_->addItem(line);
    }
    if (remotePeers_.isEmpty()) {
        onlineMembersList_->addItem(QStringLiteral("(no peers yet)"));
    }
}

void CollaborationPanelWidget::appendActivityLine(const QString &line)
{
    activityList_->insertItem(0, line);
    while (activityList_->count() > 80) {
        delete activityList_->takeItem(activityList_->count() - 1);
    }
}

QString CollaborationPanelWidget::relativeWorkspacePath(const QString &absolutePath) const
{
    if (absolutePath.isEmpty()) {
        return {};
    }
    if (workspaceRootAbsolute_.isEmpty()) {
        return QFileInfo(absolutePath).fileName();
    }
    QString rel = QDir(workspaceRootAbsolute_).relativeFilePath(absolutePath);
    if (rel.startsWith(QStringLiteral(".."))) {
        return QFileInfo(absolutePath).fileName();
    }
    return QDir::fromNativeSeparators(rel);
}

QString CollaborationPanelWidget::displayNameForClient(const QString &clientId) const
{
    if (clientId.size() <= 10) {
        return clientId;
    }
    return clientId.left(6) + QStringLiteral("…") + clientId.right(4);
}

void CollaborationPanelWidget::applyRosterJson(const QJsonObject &obj)
{
    remotePeers_.clear();
    const QJsonArray members = obj.value(QStringLiteral("members")).toArray();
    for (const QJsonValue &v : members) {
        if (!v.isObject()) {
            continue;
        }
        const QJsonObject m = v.toObject();
        const QString id = m.value(QStringLiteral("clientId")).toString();
        if (id.isEmpty()) {
            continue;
        }
        CollaboratorPeer p;
        p.clientId = id;
        p.currentFile = m.value(QStringLiteral("filePath")).toString();
        remotePeers_.insert(id, p);
    }
    rebuildOnlineList();
    appendActivityLine(QStringLiteral("Roster synced (%1 online).").arg(remotePeers_.size()));
    emit collaborationRosterSynced();
}

void CollaborationPanelWidget::applyUserJoined(const QJsonObject &obj)
{
    const QJsonObject user = obj.value(QStringLiteral("user")).toObject();
    QString id = user.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        id = obj.value(QStringLiteral("clientId")).toString();
    }
    if (id.isEmpty() || id == localClientId_) {
        rebuildOnlineList();
        return;
    }
    CollaboratorPeer p;
    p.clientId = id;
    p.currentFile.clear();
    remotePeers_.insert(id, p);
    rebuildOnlineList();
    appendActivityLine(QStringLiteral("%1 joined").arg(displayNameForClient(id)));
}

void CollaborationPanelWidget::applyUserLeft(const QJsonObject &obj)
{
    const QJsonObject user = obj.value(QStringLiteral("user")).toObject();
    QString id = user.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        id = obj.value(QStringLiteral("clientId")).toString();
    }
    if (id.isEmpty()) {
        return;
    }
    remotePeers_.remove(id);
    rebuildOnlineList();
    appendActivityLine(QStringLiteral("%1 left").arg(displayNameForClient(id)));
}

void CollaborationPanelWidget::applyCurrentFileChanged(const QJsonObject &obj)
{
    const QString uid = obj.value(QStringLiteral("userId")).toString();
    if (uid.isEmpty()) {
        return;
    }
    if (!remotePeers_.contains(uid)) {
        CollaboratorPeer p;
        p.clientId = uid;
        p.currentFile = obj.value(QStringLiteral("filePath")).toString();
        remotePeers_.insert(uid, p);
    } else {
        remotePeers_[uid].currentFile = obj.value(QStringLiteral("filePath")).toString();
    }
    rebuildOnlineList();
    const QString fp = obj.value(QStringLiteral("filePath")).toString();
    appendActivityLine(
        QStringLiteral("%1 → %2").arg(displayNameForClient(uid), fp.isEmpty() ? QStringLiteral("(cleared)") : fp));
}

void CollaborationPanelWidget::onWebSocketMessage(const QString &text)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    const QJsonObject o = doc.object();
    const QString t = o.value(QStringLiteral("type")).toString();
    if (t == QStringLiteral("server.welcome")) {
        const QString cid = o.value(QStringLiteral("clientId")).toString();
        if (!cid.isEmpty()) {
            localClientId_ = cid;
        }
        appendActivityLine(QStringLiteral("Connected (%1).").arg(displayNameForClient(localClientId_)));
        return;
    }
    if (t == QStringLiteral("presence.roster")) {
        applyRosterJson(o);
        return;
    }
    if (t == QStringLiteral("presence.user_joined")) {
        applyUserJoined(o);
        return;
    }
    if (t == QStringLiteral("presence.user_left")) {
        applyUserLeft(o);
        return;
    }
    if (t == QStringLiteral("presence.current_file_changed")) {
        applyCurrentFileChanged(o);
        return;
    }
    if (t == QStringLiteral("collab.file_saved")) {
        const QString who = o.value(QStringLiteral("clientId")).toString();
        const QString fp = o.value(QStringLiteral("filePath")).toString();
        const QString ts = o.value(QStringLiteral("timestamp")).toString();
        if (who != localClientId_) {
            appendActivityLine(
                QStringLiteral("[%2] %1 saved %3")
                    .arg(displayNameForClient(who), ts.isEmpty() ? QStringLiteral("?") : ts, fp));
        }
        return;
    }
    if (t == QStringLiteral("file.updated") || t == QStringLiteral("editor.patch")) {
        const QString who = o.value(QStringLiteral("clientId")).toString();
        if (who == localClientId_) {
            return;
        }
        const QString fp = o.value(QStringLiteral("filePath")).toString();
        const QString content = o.value(QStringLiteral("content")).toString();
        appendActivityLine(QStringLiteral("%1 updated %2").arg(displayNameForClient(who), fp));
        emit remoteFileUpdated(QDir(workspaceRootAbsolute_).filePath(fp), content);
        return;
    }
    if (t == QStringLiteral("editor.cursor") || t == QStringLiteral("editor.soft_lock")) {
        const QString who = o.value(QStringLiteral("clientId")).toString();
        if (who == localClientId_) {
            return;
        }
        const QString name = o.value(QStringLiteral("username")).toString(displayNameForClient(who));
        const QString fp = o.value(QStringLiteral("filePath")).toString();
        const QJsonObject cursor = o.value(QStringLiteral("cursor")).toObject();
        const int line = cursor.value(QStringLiteral("line")).toInt(1);
        const int column = cursor.value(QStringLiteral("column")).toInt(1);
        appendActivityLine(QStringLiteral("%1 editing %2:%3").arg(name).arg(line).arg(column));
        emit remoteCursorMoved(QDir(workspaceRootAbsolute_).filePath(fp), name, line, column);
        return;
    }
    if (t == QStringLiteral("heartbeat.ack")) {
        return;
    }
}
#endif
