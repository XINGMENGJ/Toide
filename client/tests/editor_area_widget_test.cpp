#include <QtTest/QtTest>

#include "editor/editor_area_widget.h"

#include <QTabWidget>
#include <QTemporaryDir>

class EditorAreaWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void openFileCreatesOneTabAndReusesIt();
};

void EditorAreaWidgetTest::openFileCreatesOneTabAndReusesIt()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto filePath = directory.filePath(QStringLiteral("main.cpp"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("int main() {}\n");
    file.close();

    EditorAreaWidget editorArea;
    auto *tabs = editorArea.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);

    QVERIFY(editorArea.openFile(filePath));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->tabText(0), QStringLiteral("main.cpp"));

    QVERIFY(editorArea.openFile(filePath));
    QCOMPARE(tabs->count(), 1);
    QCOMPARE(tabs->currentIndex(), 0);
}

QTEST_MAIN(EditorAreaWidgetTest)

#include "editor_area_widget_test.moc"
