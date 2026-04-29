#include <QtTest/QtTest>

#include "app/main_window.h"

#include <QMenu>
#include <QToolBar>
#include <QWidget>

class MainWindowSmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void mainWindowHasExpectedShell();
};

void MainWindowSmokeTest::mainWindowHasExpectedShell()
{
    MainWindow window;

    QCOMPARE(window.windowTitle(), QStringLiteral("Toide"));
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("fileExplorerPanel")) != nullptr);
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("editorTabs")) != nullptr);
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("collaborationPanel")) != nullptr);
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("outputTabs")) != nullptr);
    QVERIFY(window.findChild<QMenu *>(QStringLiteral("fileMenu")) != nullptr);
    QVERIFY(window.findChild<QToolBar *>(QStringLiteral("mainToolBar")) != nullptr);
}

QTEST_MAIN(MainWindowSmokeTest)

#include "main_window_smoke_test.moc"
