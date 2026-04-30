#include <QtTest/QtTest>

#include "collaboration/collaboration_panel_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>

class CollaborationPanelWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void showsDisconnectedByDefault();
    void showsOnlineAndOfflineStatus();
};

void CollaborationPanelWidgetTest::showsDisconnectedByDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab.ini")));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("serverConnectionStatusLabel"));
    QVERIFY(statusLabel != nullptr);
    QVERIFY(statusLabel->text().contains(QStringLiteral("Not connected")));

    auto *checkButton = widget.findChild<QPushButton *>(QStringLiteral("checkServerConnectionButton"));
    QVERIFY(checkButton != nullptr);
    QVERIFY(checkButton->text().contains(QStringLiteral("Check")));

    auto *urlEdit = widget.findChild<QLineEdit *>(QStringLiteral("serverBaseUrlLineEdit"));
    QVERIFY(urlEdit != nullptr);
    QVERIFY(!urlEdit->text().isEmpty());
}

void CollaborationPanelWidgetTest::showsOnlineAndOfflineStatus()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CollaborationPanelWidget widget(nullptr, dir.filePath(QStringLiteral("collab2.ini")));

    auto *statusLabel = widget.findChild<QLabel *>(QStringLiteral("serverConnectionStatusLabel"));
    QVERIFY(statusLabel != nullptr);

    widget.setServerStatus(true, QStringLiteral("Online"));
    QVERIFY(statusLabel->text().contains(QStringLiteral("Online")));

    widget.setServerStatus(false, QStringLiteral("Offline: refused"));
    QVERIFY(statusLabel->text().contains(QStringLiteral("Offline")));
    QVERIFY(statusLabel->text().contains(QStringLiteral("refused")));
}

QTEST_MAIN(CollaborationPanelWidgetTest)

#include "collaboration_panel_widget_test.moc"
