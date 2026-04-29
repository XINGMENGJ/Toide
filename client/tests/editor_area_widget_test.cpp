#include <QtTest/QtTest>

#include "editor/editor_area_widget.h"
#include "editor/editor_tab.h"

#include <QTabWidget>
#include <QTemporaryDir>

class EditorAreaWidgetTest final : public QObject {
    Q_OBJECT

private slots:
    void openFileCreatesOneTabAndReusesIt();
    void saveCurrentFilePersistsCurrentTab();
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

void EditorAreaWidgetTest::saveCurrentFilePersistsCurrentTab()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto filePath = directory.filePath(QStringLiteral("notes.txt"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("before\n");
    file.close();

    EditorAreaWidget editorArea;
    QVERIFY(editorArea.openFile(filePath));

    auto *tabs = editorArea.findChild<QTabWidget *>(QStringLiteral("editorTabs"));
    QVERIFY(tabs != nullptr);

    auto *editor = qobject_cast<EditorTab *>(tabs->currentWidget());
    QVERIFY(editor != nullptr);
    editor->setText(QStringLiteral("after\n"));

    QVERIFY(editorArea.saveCurrentFile());
    QVERIFY(!editor->isDirty());

    QFile savedFile(filePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(savedFile.readAll()), QStringLiteral("after\n"));
}

QTEST_MAIN(EditorAreaWidgetTest)

#include "editor_area_widget_test.moc"
