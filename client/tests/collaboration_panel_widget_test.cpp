#include <QtTest/QtTest>

#include "collaboration/collaboration_panel_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>

class CollaborationPanelWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void showsDisconnectedByDefault();
    void showsOnlineAndOfflineStatus();
    void fileUpdatedWithoutContentDoesNotEmitRemoteUpdate();
    void editorPatchWithContentEmitsRemoteUpdate();
    void rosterUsesServerUsernamesInMemberList();
};

void CollaborationPanelWidgetTest::showsDisconnectedByDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab.ini")));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("serverConnectionStatusLabel"));
    QVERIFY(statusLabel != nullptr);
    QVERIFY(statusLabel->text().contains(QStringLiteral("未连接")));

    auto *checkButton = widget.findChild<QPushButton *>(QStringLiteral("checkServerConnectionButton"));
    QVERIFY(checkButton != nullptr);
    QVERIFY(checkButton->text().contains(QStringLiteral("检测")));

    auto *urlEdit = widget.findChild<QLineEdit *>(QStringLiteral("serverBaseUrlLineEdit"));
    QVERIFY(urlEdit != nullptr);
    QVERIFY(!urlEdit->text().isEmpty());

    auto *channelLabel = widget.findChild<QLabel *>(QStringLiteral("collaborationChannelStatusLabel"));
    QVERIFY(channelLabel != nullptr);
#ifdef TOIDE_HAVE_QT_WEBSOCKETS
    QVERIFY(channelLabel->text().contains(QStringLiteral("未连接"), Qt::CaseInsensitive));
    auto *channelButton = widget.findChild<QPushButton *>(QStringLiteral("collaborationChannelButton"));
    QVERIFY(channelButton != nullptr);
    QVERIFY(!channelButton->isHidden());
    auto *onlineList = widget.findChild<QListWidget *>(QStringLiteral("collaborationOnlineMembersList"));
    QVERIFY(onlineList != nullptr);
    QVERIFY(!onlineList->isHidden());
#else
    QVERIFY(channelLabel->text().contains(QStringLiteral("不可用"), Qt::CaseInsensitive));
#endif
}

void CollaborationPanelWidgetTest::showsOnlineAndOfflineStatus()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab2.ini")));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("serverConnectionStatusLabel"));
    QVERIFY(statusLabel != nullptr);

    widget.setServerStatus(true, QStringLiteral("Online"));
    QVERIFY(statusLabel->text().contains(QStringLiteral("在线")));

    widget.setServerStatus(false, QStringLiteral("离线：连接被拒绝"));
    QVERIFY(statusLabel->text().contains(QStringLiteral("离线")));
    QVERIFY(statusLabel->text().contains(QStringLiteral("拒绝")));
}

void CollaborationPanelWidgetTest::fileUpdatedWithoutContentDoesNotEmitRemoteUpdate()
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    QSKIP("Qt WebSockets not available");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab3.ini")));
    widget.setWorkspaceRoot(dir.path());
    QSignalSpy updateSpy(&widget, &CollaborationPanelWidget::remoteFileUpdated);

    const QString message =
        QStringLiteral(R"({"type":"file.updated","clientId":"peer-1","filePath":"notes.txt","version":2})");
    QVERIFY(QMetaObject::invokeMethod(&widget, "onWebSocketMessage", Qt::DirectConnection, Q_ARG(QString, message)));

    QCOMPARE(updateSpy.count(), 0);
#endif
}

void CollaborationPanelWidgetTest::editorPatchWithContentEmitsRemoteUpdate()
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    QSKIP("Qt WebSockets not available");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab4.ini")));
    widget.setWorkspaceRoot(dir.path());
    QSignalSpy updateSpy(&widget, &CollaborationPanelWidget::remoteFileUpdated);

    const QString message =
        QStringLiteral(R"({"type":"editor.patch","clientId":"peer-1","filePath":"notes.txt","content":"hello"})");
    QVERIFY(QMetaObject::invokeMethod(&widget, "onWebSocketMessage", Qt::DirectConnection, Q_ARG(QString, message)));

    QCOMPARE(updateSpy.count(), 1);
    QCOMPARE(updateSpy.takeFirst().at(1).toString(), QStringLiteral("hello"));
#endif
}

void CollaborationPanelWidgetTest::rosterUsesServerUsernamesInMemberList()
{
#ifndef TOIDE_HAVE_QT_WEBSOCKETS
    QSKIP("Qt WebSockets not available");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab5.ini")));
    widget.setWorkspaceRoot(dir.path());

    const QString message = QStringLiteral(
        R"({"type":"presence.roster","projectId":"p","members":[{"clientId":"c-remote","username":"张三","filePath":"a.cpp"}]})");
    QVERIFY(QMetaObject::invokeMethod(&widget, "onWebSocketMessage", Qt::DirectConnection, Q_ARG(QString, message)));

    auto *onlineList = widget.findChild<QListWidget *>(QStringLiteral("collaborationOnlineMembersList"));
    QVERIFY(onlineList != nullptr);
    bool found = false;
    for (int i = 0; i < onlineList->count(); ++i) {
        if (onlineList->item(i)->text().contains(QStringLiteral("张三"))) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
#endif
}

QTEST_MAIN(CollaborationPanelWidgetTest)

#include "collaboration_panel_widget_test.moc"
