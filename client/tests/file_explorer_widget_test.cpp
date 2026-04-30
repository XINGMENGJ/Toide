#include <QtTest/QtTest>

#include "file_explorer/file_explorer_widget.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>

class FileExplorerWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void createFileCreatesAndEmitsOpenRequest();
    void createFolderCreatesDirectory();
    void rejectsNestedNames();
};

void FileExplorerWidgetTest::createFileCreatesAndEmitsOpenRequest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileExplorerWidget widget;
    widget.setProjectRoot(dir.path());
    QSignalSpy openSpy(&widget, &FileExplorerWidget::fileOpenRequested);

    QString error;
    const QString path = widget.createFileInDirectory(dir.path(), QStringLiteral("new.txt"), &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(QFileInfo::exists(path));
    QCOMPARE(openSpy.count(), 1);
    QCOMPARE(openSpy.takeFirst().at(0).toString(), path);
}

void FileExplorerWidgetTest::createFolderCreatesDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileExplorerWidget widget;
    widget.setProjectRoot(dir.path());

    QString error;
    const QString path = widget.createFolderInDirectory(dir.path(), QStringLiteral("src"), &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(QFileInfo(path).isDir());
}

void FileExplorerWidgetTest::rejectsNestedNames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    FileExplorerWidget widget;
    widget.setProjectRoot(dir.path());

    QString error;
    const QString path = widget.createFileInDirectory(dir.path(), QStringLiteral("bad/name.txt"), &error);

    QVERIFY(path.isEmpty());
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(FileExplorerWidgetTest)

#include "file_explorer_widget_test.moc"
