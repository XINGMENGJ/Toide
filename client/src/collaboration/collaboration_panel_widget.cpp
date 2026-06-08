#include "collaboration/collaboration_panel_widget.h"

#include <QFileInfo>
#include <QHBoxLayout>
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
    , serverConnectionStatusLabel_(new QLabel(QStringLiteral("服务器：未连接"), this))
    , serverBaseUrlEdit_(new QLineEdit(this))
    , checkServerConnectionButton_(new QPushButton(QStringLiteral("检测连接"), this))
    , collaborationChannelStatusLabel_(new QLabel(this))
    , collaborationChannelButton_(new QPushButton(QStringLiteral("连接协作频道"), this))
    , onlineMembersCaption_(new QLabel(QStringLiteral("本项目在线成员"), this))
    , onlineMembersList_(new QListWidget(this))
    , activityCaption_(new QLabel(QStringLiteral("协作动态"), this))
    , activityList_(new QListWidget(this))
    , accountStatusLabel_(new QLabel(this))
    , accountActionButton_(new QPushButton(this))
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
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    authToken_ = endpointSettings_.authToken();
    username_ = endpointSettings_.username();
#endif
    accountStatusLabel_->setObjectName(QStringLiteral("collaborationAccountStatusLabel"));
    accountStatusLabel_->setWordWrap(true);
    accountStatusLabel_->setTextFormat(Qt::RichText);
    accountStatusLabel_->setOpenExternalLinks(false);
    accountActionButton_->setObjectName(QStringLiteral("collaborationAccountActionButton"));

    onlineMembersList_->setMinimumHeight(96);
    activityList_->setMinimumHeight(120);
    activityList_->setAlternatingRowColors(true);

#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    collaborationWsClient_ = new CollaborationWebSocketClient(this);
    collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：未连接"));
    collaborationChannelButton_->setEnabled(true);
    heartbeatTimer_.setInterval(15000);
    connect(&heartbeatTimer_, &QTimer::timeout, this, &CollaborationPanelWidget::sendHeartbeat);

    connect(collaborationWsClient_, &CollaborationWebSocketClient::connected, this, [this]() {
        heartbeatTimer_.start();
        updateCollaborationChannelUi();
        sendPresenceJoin();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::disconnected, this, [this]() {
        heartbeatTimer_.stop();
        updateCollaborationChannelUi();
        clearPresenceUi();
    });
    connect(collaborationWsClient_, &CollaborationWebSocketClient::textMessageReceived, this,
            &CollaborationPanelWidget::onWebSocketMessage);
    connect(collaborationWsClient_, &CollaborationWebSocketClient::errorOccurred, this, [this](const QString &message) {
        collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：错误（%1）").arg(message));
        collaborationChannelButton_->setText(QStringLiteral("连接协作频道"));
    });
    connect(collaborationChannelButton_, &QPushButton::clicked, this,
            &CollaborationPanelWidget::onCollaborationChannelButtonClicked);
#else
    collaborationChannelStatusLabel_->setWordWrap(true);
    collaborationChannelStatusLabel_->setText(
        QStringLiteral("协作频道：不可用。本构建未编入 Qt WebSockets，在线成员与实时协作无效。\n"
                       "请在 Qt Maintenance Tool 中为当前 Kit 使用的 Qt 版本勾选「Qt WebSockets」，"
                       "然后在工程中「运行 qmake」并执行完整重新构建。"));
    collaborationChannelButton_->setVisible(false);
    onlineMembersCaption_->setVisible(false);
    onlineMembersList_->setVisible(false);
    activityCaption_->setVisible(false);
    activityList_->setVisible(false);
#endif

    auto *titleLabel = new QLabel(QStringLiteral("协作"), this);
    titleLabel->setObjectName(QStringLiteral("collaborationPanelTitleLabel"));

    auto *accountRow = new QHBoxLayout();
    accountRow->setContentsMargins(0, 0, 0, 0);
    accountRow->addWidget(accountStatusLabel_, 1);
    accountRow->addWidget(accountActionButton_, 0, Qt::AlignTop);
    connect(accountActionButton_, &QPushButton::clicked, this, [this]() {
        if (!endpointSettings_.authToken().isEmpty()) {
            clearAuthSession();
            return;
        }
        emit loginRequested();
    });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(titleLabel);
    layout->addLayout(accountRow);
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
        serverConnectionStatusLabel_->setText(QStringLiteral("服务器：正在检测…"));
        emit serverHealthCheckRequested(QUrl(endpointSettings_.serverBaseUrl()));
    });

    refreshAccountUi();
}

void CollaborationPanelWidget::setServerStatus(bool online, const QString &message)
{
    serverConnectionStatusLabel_->setText(QStringLiteral("服务器：%1").arg(online ? QStringLiteral("在线") : message));
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

void CollaborationPanelWidget::setCollaborationProjectKey(const QString &projectKey)
{
    if (projectKey.isEmpty()) {
        collaborationProjectId_ = QStringLiteral("default");
        return;
    }
    collaborationProjectId_ = projectKey;
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
    selfRow.username = username_;
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

void CollaborationPanelWidget::notifyLocalFileSynced(const QString &absoluteFilePath, qint64 version)
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    Q_UNUSED(absoluteFilePath);
    Q_UNUSED(version);
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
    if (version >= 0) {
        o[QStringLiteral("version")] = version;
    }
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
#endif
}

void CollaborationPanelWidget::setAuthSession(const QString &token, const QString &username)
{
    endpointSettings_.setAuthSession(token, username);
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    authToken_ = token;
    username_ = username;
#endif
    refreshAccountUi();
}

void CollaborationPanelWidget::clearAuthSession()
{
    endpointSettings_.clearAuthSession();
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    authToken_.clear();
    username_.clear();
    if (collaborationWsClient_ != nullptr && collaborationWsClient_->isConnected()) {
        collaborationWsClient_->disconnectFromServer();
        updateCollaborationChannelUi();
    }
#endif
    refreshAccountUi();
}

QString CollaborationPanelWidget::authStatusLine() const
{
    if (endpointSettings_.authToken().isEmpty()) {
        return QStringLiteral("未登录");
    }
    const QString u = endpointSettings_.username();
    return u.isEmpty() ? QStringLiteral("已登录")
                       : QStringLiteral(R"(已登录："%1")").arg(u);
}

bool CollaborationPanelWidget::isSignedIn() const
{
    return !endpointSettings_.authToken().isEmpty();
}

QString CollaborationPanelWidget::collaborationProjectKey() const
{
    return collaborationProjectId_;
}

QString CollaborationPanelWidget::authBearerToken() const
{
    return endpointSettings_.authToken();
}

QString CollaborationPanelWidget::collaborationServerBaseUrl() const
{
    return endpointSettings_.serverBaseUrl();
}

void CollaborationPanelWidget::refreshAccountUi()
{
    const bool hasToken = !endpointSettings_.authToken().isEmpty();
    const QString u = endpointSettings_.username();
    if (hasToken && !u.isEmpty()) {
        accountStatusLabel_->setText(
            QStringLiteral("<p style=\"margin:0\"><b>已登录</b><br/>用户：%1</p>").arg(u.toHtmlEscaped()));
    } else if (hasToken) {
        accountStatusLabel_->setText(QStringLiteral("<p style=\"margin:0\"><b>已登录</b>（会话有效）</p>"));
    } else {
        accountStatusLabel_->setText(QStringLiteral(
            "<p style=\"margin:0\"><b>未登录</b><br/><span style=\"color:#666\">请先登录以使用协作功能。</span></p>"));
    }
    accountActionButton_->setText(hasToken ? QStringLiteral("退出登录") : QStringLiteral("登录…"));
    emit authSessionChanged();
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
    constexpr int kMaxPatchChars = 16384;
    if (text.size() <= kMaxPatchChars) {
        o[QStringLiteral("content")] = text;
    } else {
        o[QStringLiteral("contentOmitted")] = true;
        o[QStringLiteral("contentLength")] = text.size();
    }
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
        connected ? QStringLiteral("断开协作频道") : QStringLiteral("连接协作频道"));
    if (connected) {
        collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：已连接"));
    } else if (collaborationWsClient_->isReconnectEnabled()) {
        collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：重连中…"));
    } else {
        collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：未连接"));
    }
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

    if (authToken_.isEmpty()) {
        collaborationChannelStatusLabel_->setText(
            QStringLiteral("协作频道：请先登录（见上方账号区域）"));
        return;
    }

    const QUrl http(endpointSettings_.serverBaseUrl());
    const QUrl ws = CollaborationWebSocketClient::buildCollaborationWebSocketUrl(
        http, collaborationProjectId_, authToken_, localClientId_);

    if (ws.scheme().isEmpty() || ws.host().isEmpty()) {
        collaborationChannelStatusLabel_->setText(
            QStringLiteral("协作频道：服务器地址无法用于 WebSocket"));
        return;
    }

    collaborationChannelStatusLabel_->setText(QStringLiteral("协作频道：正在连接…"));
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

void CollaborationPanelWidget::sendHeartbeat()
{
    if (collaborationWsClient_ == nullptr || !collaborationWsClient_->isConnected()) {
        return;
    }
    QJsonObject o;
    o[QStringLiteral("type")] = QStringLiteral("heartbeat");
    o[QStringLiteral("clientId")] = localClientId_;
    o[QStringLiteral("projectId")] = collaborationProjectId_;
    collaborationWsClient_->sendTextMessage(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

void CollaborationPanelWidget::clearPresenceUi()
{
    remotePeers_.clear();
    onlineMembersList_->clear();
    appendActivityLine(QStringLiteral("已断开协作频道。"));
}

void CollaborationPanelWidget::rebuildOnlineList()
{
    onlineMembersList_->clear();
    for (auto it = remotePeers_.constBegin(); it != remotePeers_.constEnd(); ++it) {
        const QString &cid = it.key();
        const CollaboratorPeer &p = it.value();
        QString line = displayNameForClient(cid);
        if (cid == localClientId_) {
            line.append(QStringLiteral("（我）"));
        }
        if (p.currentFile.isEmpty()) {
            line.append(QStringLiteral(" — "));
            line.append(QStringLiteral("（未打开文件）"));
        } else {
            line.append(QStringLiteral(" — "));
            line.append(p.currentFile);
        }
        onlineMembersList_->addItem(line);
    }
    if (remotePeers_.isEmpty()) {
        onlineMembersList_->addItem(QStringLiteral("（暂无其他成员）"));
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
    if (clientId == localClientId_ && !username_.isEmpty()) {
        return username_;
    }
    const auto it = remotePeers_.constFind(clientId);
    if (it != remotePeers_.constEnd() && !it->username.isEmpty()) {
        return it->username;
    }
    if (clientId.size() <= 10) {
        return clientId;
    }
    return clientId.left(6) + QStringLiteral("…") + clientId.right(4);
}

bool CollaborationPanelWidget::hasStringContent(const QJsonObject &obj)
{
    const QJsonValue content = obj.value(QStringLiteral("content"));
    return content.isString();
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
        p.username = m.value(QStringLiteral("username")).toString();
        remotePeers_.insert(id, p);
    }
    rebuildOnlineList();
    appendActivityLine(QStringLiteral("成员列表已同步（%1 人在线）。").arg(remotePeers_.size()));
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
    p.username = user.value(QStringLiteral("username")).toString();
    remotePeers_.insert(id, p);
    rebuildOnlineList();
    appendActivityLine(QStringLiteral("%1 加入").arg(displayNameForClient(id)));
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
    appendActivityLine(QStringLiteral("%1 离开").arg(displayNameForClient(id)));
}

void CollaborationPanelWidget::applyCurrentFileChanged(const QJsonObject &obj)
{
    QString peerKey = obj.value(QStringLiteral("clientId")).toString();
    if (peerKey.isEmpty()) {
        peerKey = obj.value(QStringLiteral("userId")).toString();
    }
    if (peerKey.isEmpty() || peerKey == localClientId_) {
        return;
    }
    const QString uname = obj.value(QStringLiteral("username")).toString();
    if (!remotePeers_.contains(peerKey)) {
        CollaboratorPeer p;
        p.clientId = peerKey;
        p.currentFile = obj.value(QStringLiteral("filePath")).toString();
        p.username = uname;
        remotePeers_.insert(peerKey, p);
    } else {
        remotePeers_[peerKey].currentFile = obj.value(QStringLiteral("filePath")).toString();
        if (!uname.isEmpty()) {
            remotePeers_[peerKey].username = uname;
        }
    }
    rebuildOnlineList();
    const QString fp = obj.value(QStringLiteral("filePath")).toString();
    appendActivityLine(
        QStringLiteral("%1 → %2").arg(displayNameForClient(peerKey), fp.isEmpty() ? QStringLiteral("（已清除）") : fp));
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
        appendActivityLine(QStringLiteral("已连接（%1）。").arg(displayNameForClient(localClientId_)));
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
        const QString whoLabel = o.value(QStringLiteral("username")).toString();
        if (who != localClientId_) {
            appendActivityLine(
                QStringLiteral("[%2] %1 保存了 %3")
                    .arg(whoLabel.isEmpty() ? displayNameForClient(who) : whoLabel,
                         ts.isEmpty() ? QStringLiteral("?") : ts,
                         fp));
        }
        return;
    }
    if (t == QStringLiteral("file.updated") || t == QStringLiteral("editor.patch")) {
        const QString who = o.value(QStringLiteral("clientId")).toString();
        if (who == localClientId_) {
            return;
        }
        const QString whoLabel = o.value(QStringLiteral("username")).toString();
        const QString whoDisp = whoLabel.isEmpty() ? displayNameForClient(who) : whoLabel;
        const QString fp = o.value(QStringLiteral("filePath")).toString();
        if (t == QStringLiteral("editor.patch") && o.value(QStringLiteral("contentOmitted")).toBool()) {
            appendActivityLine(
                QStringLiteral("%1 正在编辑 %2（内容未同步：过大）").arg(whoDisp, fp));
            return;
        }
        if (!hasStringContent(o)) {
            const qint64 version = o.value(QStringLiteral("version")).toVariant().toLongLong();
            const QString versionText = version > 0 ? QStringLiteral("，版本 %1").arg(version) : QString();
            appendActivityLine(QStringLiteral("%1 更新了 %2%3").arg(whoDisp, fp, versionText));
            return;
        }
        const QString content = o.value(QStringLiteral("content")).toString();
        appendActivityLine(QStringLiteral("%1 更新了 %2").arg(whoDisp, fp));
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
        appendActivityLine(QStringLiteral("%1 正在编辑 %2:%3:%4").arg(name, fp).arg(line).arg(column));
        emit remoteCursorMoved(QDir(workspaceRootAbsolute_).filePath(fp), name, line, column);
        return;
    }
    if (t == QStringLiteral("heartbeat.ack")) {
        return;
    }
}
#endif
