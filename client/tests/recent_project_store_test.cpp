#include <QtTest/QtTest>

#include "workspace/recent_project_store.h"

#include <QTemporaryDir>

class RecentProjectStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void addProjectKeepsNewestFirstAndDeduplicates();
    void addProjectKeepsOnlyConfiguredLimit();
};

void RecentProjectStoreTest::addProjectKeepsNewestFirstAndDeduplicates()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    RecentProjectStore store(directory.filePath(QStringLiteral("recent.ini")), 5);

    store.addProject(QStringLiteral("E:/projects/alpha"));
    store.addProject(QStringLiteral("E:/projects/beta"));
    store.addProject(QStringLiteral("E:/projects/alpha"));

    QCOMPARE(store.recentProjects(), QStringList({
        QStringLiteral("E:/projects/alpha"),
        QStringLiteral("E:/projects/beta"),
    }));
}

void RecentProjectStoreTest::addProjectKeepsOnlyConfiguredLimit()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    RecentProjectStore store(directory.filePath(QStringLiteral("recent.ini")), 2);

    store.addProject(QStringLiteral("E:/projects/one"));
    store.addProject(QStringLiteral("E:/projects/two"));
    store.addProject(QStringLiteral("E:/projects/three"));

    QCOMPARE(store.recentProjects(), QStringList({
        QStringLiteral("E:/projects/three"),
        QStringLiteral("E:/projects/two"),
    }));
}

QTEST_MAIN(RecentProjectStoreTest)

#include "recent_project_store_test.moc"
